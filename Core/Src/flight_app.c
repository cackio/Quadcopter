#include "flight_app.h"
#include "SEGGER_RTT.h"
#include "ano_protocol.h"
#include "attitude.h"
#include "bsp.h"
#include "flight_config.h"
#include "imu_driver.h"
#include "sensor_calibration.h"
#include "tim.h"
#include "timebase.h"
#include "ucos_ii.h"
#include "usart.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define ATTITUDE_PRIO 7U
#define TELEMETRY_PRIO 9U
static OS_STK attitude_stack[768], telemetry_stack[768];
static OS_EVENT *attitude_sem, *telemetry_sem;
static volatile uint8_t attitude_pending, telemetry_pending;
static volatile uint32_t release_missed, tx_dropped;
static volatile uint32_t attitude_stack_free, telemetry_stack_free;
static volatile uint8_t tx_busy;
static uint8_t tx_buffer[96]; /* owned by UART until TxCplt/abort */
static uint32_t tx_started_us, tx_deadline_us;
static FlightSnapshot published, working;
static AttitudeFilter filter;
static GyroCalibration gyro_cal;
static uint32_t last_sample_us, warmup_samples;
static uint8_t imu_ok, mag_present, mag_valid, config_ok, initialized, have_time;
static const float imu_to_body[3][3] = CAL_IMU_TO_BODY;
static const float mag_to_body[3][3] = CAL_MAG_TO_BODY;
static const float accel_bias[3] = CAL_ACCEL_BIAS_G, accel_scale[3] = CAL_ACCEL_SCALE;
static const float mag_bias[3] = CAL_MAG_BIAS_RAW, mag_matrix[3][3] = CAL_MAG_MATRIX;
static float display_basis[4] = FLIGHT_DISPLAY_BASIS_Q;

static int calibration_valid(void)
{
    unsigned i, j;
    float a, b, c, d, e, f;
    if (!isfinite(FLIGHT_BETA) || FLIGHT_BETA < 0 || !Attitude_MatrixValid(imu_to_body) ||
        !Attitude_MatrixValid(mag_to_body) || !Attitude_Normalize(display_basis))
        return 0;
    for (i = 0; i < 3; ++i)
    {
        if (!isfinite(accel_bias[i]) || !isfinite(accel_scale[i]) || accel_scale[i] <= 0 ||
            !isfinite(mag_bias[i]))
            return 0;
        for (j = 0; j < 3; ++j)
            if (!isfinite(mag_matrix[i][j]) || fabsf(mag_matrix[i][j] - mag_matrix[j][i]) > 1.0e-5f)
                return 0;
    }
    /* Positive definite soft-iron matrix: Cholesky, no absolute determinant
     * cutoff (raw-count -> unit-field coefficients are naturally small). */
    a = mag_matrix[0][0];
    if (a <= 0)
        return 0;
    a = sqrtf(a);
    b = mag_matrix[1][0] / a;
    c = mag_matrix[2][0] / a;
    d = mag_matrix[1][1] - b * b;
    if (d <= 0)
        return 0;
    d = sqrtf(d);
    e = (mag_matrix[2][1] - b * c) / d;
    f = mag_matrix[2][2] - c * c - e * e;
    return isfinite(f) && f > 0;
}
void Flight_GetSnapshot(FlightSnapshot *out)
{
    OS_CPU_SR cpu_sr = 0;
    OS_ENTER_CRITICAL();
    *out = published;
    out->release_missed = release_missed;
    out->tx_dropped = tx_dropped;
    out->attitude_stack_free = attitude_stack_free;
    out->telemetry_stack_free = telemetry_stack_free;
    OS_EXIT_CRITICAL();
}
static void publish(void)
{
    OS_CPU_SR cpu_sr = 0;
    OS_ENTER_CRITICAL();
    working.release_missed = release_missed;
    working.tx_dropped = tx_dropped;
    published = working;
    OS_EXIT_CRITICAL();
}
static void release(OS_EVENT *sem, volatile uint8_t *pending, int attitude)
{
    if (*pending)
    {
        if (attitude)
            ++release_missed;
        return;
    }
    *pending = 1;
    if (OSSemPost(sem) != OS_ERR_NONE)
    {
        *pending = 0;
        if (attitude)
            ++release_missed;
    }
}
void Flight_TimerISR(void)
{
    static uint16_t telemetry_divider;
    uint16_t divisor = (FLIGHT_TELEMETRY_MODE == FLIGHT_TELEM_DIAG) ? FLIGHT_IMU_HZ / FLIGHT_DIAG_HZ
                                                                    : FLIGHT_IMU_HZ / FLIGHT_ANO_HZ;
    if (OSRunning != OS_TRUE || !attitude_sem || !telemetry_sem)
        return;
    release(attitude_sem, &attitude_pending, 1);
    if (++telemetry_divider >= divisor)
    {
        telemetry_divider = 0;
        release(telemetry_sem, &telemetry_pending, 0);
    }
}
static float length3(const float v[3])
{
    return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}
static void read_magnetometer(void)
{
    int16_t raw[3];
    int result;
    unsigned i, j;
    float centered[3], corrected[3];
    if (!mag_present)
        return;
    result = Magnetometer_Read(raw);
    if (result < 0)
    {
        mag_valid = 0;
        if (result == SENSOR_READ_ERROR)
            ++working.i2c_errors;
        return;
    }
    if (result != SENSOR_NEW)
        return;
    memcpy(working.mag_raw, raw, sizeof(raw));
    ++working.mag_sequence;
    working.mag_timestamp_us = BSP_TimeUs();
    mag_valid = 1;
    if (!config_ok)
        return;
    for (i = 0; i < 3; ++i)
        centered[i] = (float)raw[i] - mag_bias[i];
    for (i = 0; i < 3; ++i)
    {
        corrected[i] = 0;
        for (j = 0; j < 3; ++j)
            corrected[i] += mag_matrix[i][j] * centered[j];
    }
    Attitude_Rotate(mag_to_body, corrected, working.mag_body);
}
static void process_sample(void)
{
    ImuRawSample raw;
    float native_acc[3], native_dps[3], native_rad[3], norm;
    float *mag = NULL;
    uint32_t now;
    unsigned i;
    int result, mode, acc_trusted;
    working.flags = 0;
    if (imu_ok)
        working.flags |= STATUS_IMU_OK;
    if (mag_present)
        working.flags |= STATUS_MAG_PRESENT;
    if (CAL_MAG_VALID)
        working.flags |= STATUS_MAG_CALIBRATED;
    if (FLIGHT_IMU_AXES_CONFIRMED && FLIGHT_MAG_AXES_CONFIRMED)
        working.flags |= STATUS_AXES_CONFIRMED;
    if (!config_ok)
        working.flags |= STATUS_CONFIG_ERROR;
    if (gyro_cal.ready)
        working.flags |= STATUS_GYRO_CALIBRATED;
    result = imu_ok ? IMU_Read(&raw) : SENSOR_READ_ERROR;
    now = BSP_TimeUs();
    read_magnetometer();
    /* Read the clock after mag polling so a fresh mag sample cannot appear
     * to come from the future in unsigned age arithmetic. */
    working.mag_age_us =
        working.mag_sequence ? Time_ElapsedUs(BSP_TimeUs(), working.mag_timestamp_us) : UINT32_MAX;
    Time_ExpireUs(BSP_TimeUs(), working.mag_timestamp_us, FLIGHT_MAG_MAX_AGE_US, &mag_valid);
    if (mag_present && (!mag_valid || working.mag_age_us > FLIGHT_MAG_MAX_AGE_US))
        working.flags |= STATUS_MAG_REJECTED;
    if (result != SENSOR_NEW)
    {
        if (result == SENSOR_READ_ERROR)
        {
            ++working.i2c_errors;
            working.flags |= STATUS_IMU_ERROR;
        }
        return;
    }
    working.flags |= STATUS_IMU_NEW;
    ++working.sequence;
    working.timestamp_us = now;
    working.dt_us = have_time ? Time_ElapsedUs(now, last_sample_us) : 0;
    last_sample_us = now;
    have_time = 1;
    if (working.dt_us)
    {
        ++working.dt_total;
        if (working.dt_us >= 4500U && working.dt_us <= 5500U)
            ++working.dt_in_band;
        if (!working.dt_min_us || working.dt_us < working.dt_min_us)
            working.dt_min_us = working.dt_us;
        if (working.dt_us > working.dt_max_us)
            working.dt_max_us = working.dt_us;
    }
    for (i = 0; i < 3; ++i)
    {
        working.imu_raw[i] = raw.accel[i];
        working.imu_raw[i + 3] = raw.gyro[i];
        native_acc[i] = ((float)raw.accel[i] / 16384.0f - accel_bias[i]) * accel_scale[i];
        native_dps[i] = (float)raw.gyro[i] / 65.5f;
    }
    if (!config_ok)
        return;
    Attitude_Rotate(imu_to_body, native_acc, working.accel_g);
    if (warmup_samples < FLIGHT_GYRO_WARMUP_SAMPLES)
    {
        ++warmup_samples;
        return;
    }
    if (!GyroCalibration_Push(&gyro_cal, native_acc, native_dps, FLIGHT_GYRO_CAL_SAMPLES))
    {
        working.cal_samples = (uint16_t)gyro_cal.count;
        return;
    }
    working.cal_samples = (uint16_t)gyro_cal.count;
    working.flags |= STATUS_GYRO_CALIBRATED;
    for (i = 0; i < 3; ++i)
    {
        working.gyro_bias_dps[i] = gyro_cal.bias_dps[i];
        native_rad[i] = (native_dps[i] - gyro_cal.bias_dps[i]) * 0.017453292519943295f;
    }
    Attitude_Rotate(imu_to_body, native_rad, working.gyro_rad_s);
    norm = length3(working.accel_g);
    acc_trusted = isfinite(norm) && norm >= FLIGHT_ACCEL_MIN_G && norm <= FLIGHT_ACCEL_MAX_G;
    if (FLIGHT_FUSION_MODE == FLIGHT_FUSION_MARG && CAL_MAG_VALID && FLIGHT_IMU_AXES_CONFIRMED &&
        FLIGHT_MAG_AXES_CONFIRMED && mag_valid && working.mag_age_us <= FLIGHT_MAG_MAX_AGE_US)
    {
        norm = length3(working.mag_body);
        if (isfinite(norm) && norm >= 0.75f && norm <= 1.25f)
            mag = working.mag_body;
        else
            working.flags |= STATUS_MAG_REJECTED;
    }
    if (!initialized)
    {
        if (!acc_trusted || !Attitude_Align(&filter, working.accel_g, mag))
            return;
        initialized = 1;
    }
    else
    {
        if (!working.dt_us || working.dt_us > FLIGHT_MAX_DT_US)
        {
            ++working.dt_errors;
            working.flags |= STATUS_DT_ERROR;
            return;
        }
        mode = Attitude_Update(&filter, working.gyro_rad_s, working.accel_g, mag,
                               (float)working.dt_us * 1.0e-6f,
                               acc_trusted && FLIGHT_FUSION_MODE != FLIGHT_FUSION_GYRO);
        if (!mode)
        {
            ++working.filter_errors;
            working.flags |= STATUS_FILTER_ERROR;
            return;
        }
        if (mode >= 2)
            working.flags |= STATUS_ACCEL_USED;
        if (mode == 3)
            working.flags |= STATUS_MAG_USED;
    }
    memcpy(working.q, filter.q, sizeof(filter.q));
    Attitude_ToEuler(working.q, working.euler_deg);
    working.flags |= STATUS_ATTITUDE_VALID;
}
static void AttitudeTask(void *argument)
{
    INT8U error;
    OS_CPU_SR cpu_sr = 0;
    uint32_t start;
    (void)argument;
    for (;;)
    {
        OSSemPend(attitude_sem, 0, &error);
        if (error != OS_ERR_NONE)
        {
            OSTimeDly(1);
            continue;
        }
        OS_ENTER_CRITICAL();
        attitude_pending = 0;
        OS_EXIT_CRITICAL();
        start = BSP_TimeUs();
        process_sample();
        working.exec_us = Time_ElapsedUs(BSP_TimeUs(), start);
        if (working.exec_us > working.exec_max_us)
            working.exec_max_us = working.exec_us;
        if (working.exec_us >= FLIGHT_PERIOD_US)
            ++working.overruns;
        /* Discard a release that became obsolete during this acquisition.
         * In particular, repeated I2C timeouts must not keep this task ready
         * forever and starve the lower-priority diagnostic transmitter. */
        OS_ENTER_CRITICAL();
        if (attitude_pending)
        {
            (void)OSSemAccept(attitude_sem);
            attitude_pending = 0;
            ++release_missed;
        }
        OS_EXIT_CRITICAL();
        publish();
    }
}
static void send_snapshot(const FlightSnapshot *s)
{
    float q[4], e[3];
    uint16_t size = 0, next;
    if (tx_busy)
    {
        ++tx_dropped;
        if (Time_ElapsedUs(BSP_TimeUs(), tx_started_us) > tx_deadline_us)
        {
            /* Synchronous abort disables TX IRQs before releasing the buffer. */
            if (HAL_UART_AbortTransmit(&huart1) == HAL_OK)
                tx_busy = 0;
        }
        return;
    }
    if (FLIGHT_TELEMETRY_MODE == FLIGHT_TELEM_DIAG)
    {
        size = ANO_PackDiagnostic(s, tx_buffer, sizeof(tx_buffer));
    }
    else
    {
        if (FLIGHT_TELEMETRY_MODE == FLIGHT_TELEM_FIXED)
        {
            Attitude_FromEuler(FLIGHT_FIXED_ROLL_DEG, FLIGHT_FIXED_PITCH_DEG, FLIGHT_FIXED_YAW_DEG,
                               q);
        }
        else
        {
            if (!(s->flags & STATUS_ATTITUDE_VALID))
                return;
            memcpy(q, s->q, sizeof(q));
        }
        Attitude_Display(q, display_basis, q);
        Attitude_ToEuler(q, e);
        size = ANO_PackEuler(e, tx_buffer, sizeof(tx_buffer));
        if (FLIGHT_SEND_QUATERNION)
        {
            next = ANO_PackQuaternion(q, tx_buffer + size, sizeof(tx_buffer) - size);
            if (!next)
            {
                ++tx_dropped;
                return;
            }
            size += next;
        }
    }
    if (!size)
    {
        ++tx_dropped;
        return;
    }
    tx_started_us = BSP_TimeUs();
    tx_deadline_us =
        ((uint32_t)size * 10000000U + FLIGHT_UART_BAUD - 1U) / FLIGHT_UART_BAUD + 20000U;
    tx_busy = 1; /* Set before enabling TX interrupts: completion may preempt. */
    if (HAL_UART_Transmit_IT(&huart1, tx_buffer, size) != HAL_OK)
    {
        tx_busy = 0;
        ++tx_dropped;
    }
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uart)
{
    if (uart->Instance == USART1)
        tx_busy = 0;
}
/* Do not free a still-running TX buffer on an unrelated RX line error. */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{
    if (uart->Instance == USART1 && uart->gState == HAL_UART_STATE_READY)
        tx_busy = 0;
}
static void TelemetryTask(void *argument)
{
    INT8U error;
    OS_CPU_SR cpu_sr = 0;
    FlightSnapshot s;
    OS_STK_DATA stacks;
    uint32_t report_divider = 0;
    char text[384];
    const uint32_t rate =
        (FLIGHT_TELEMETRY_MODE == FLIGHT_TELEM_DIAG) ? FLIGHT_DIAG_HZ : FLIGHT_ANO_HZ;
    (void)argument;
    for (;;)
    {
        OSSemPend(telemetry_sem, 0, &error);
        if (error != OS_ERR_NONE)
        {
            OSTimeDly(1);
            continue;
        }
        OS_ENTER_CRITICAL();
        telemetry_pending = 0;
        OS_EXIT_CRITICAL();
        Flight_GetSnapshot(&s);
        send_snapshot(&s);
        if (++report_divider >= 2U * rate)
        {
            report_divider = 0;
            if (OSTaskStkChk(ATTITUDE_PRIO, &stacks) == OS_ERR_NONE)
                attitude_stack_free = s.attitude_stack_free = stacks.OSFree;
            if (OSTaskStkChk(TELEMETRY_PRIO, &stacks) == OS_ERR_NONE)
                telemetry_stack_free = s.telemetry_stack_free = stacks.OSFree;
            /* Stack counters have this task as their sole writer. Publish
             * separately from the attitude-owned snapshot. */
            snprintf(text, sizeof(text),
                     "seq=%lu flags=%04X cal=%u dt=%lu/%lu/%lu us exec=%lu max=%lu miss=%lu "
                     "err=%lu periodok=%lu/%lu overrun=%lu dtbad=%lu filterbad=%lu txdrop=%lu "
                     "stack=%lu/%lu words\n",
                     (unsigned long)s.sequence, s.flags, s.cal_samples, (unsigned long)s.dt_us,
                     (unsigned long)s.dt_min_us, (unsigned long)s.dt_max_us,
                     (unsigned long)s.exec_us, (unsigned long)s.exec_max_us,
                     (unsigned long)s.release_missed, (unsigned long)s.i2c_errors,
                     (unsigned long)s.dt_in_band, (unsigned long)s.dt_total,
                     (unsigned long)s.overruns, (unsigned long)s.dt_errors,
                     (unsigned long)s.filter_errors, (unsigned long)s.tx_dropped,
                     (unsigned long)s.attitude_stack_free, (unsigned long)s.telemetry_stack_free);
            SEGGER_RTT_WriteString(0, text);
        }
    }
}
void Flight_Start(void)
{
    INT8U error;
    memset(&working, 0, sizeof(working));
    working.q[0] = 1;
    filter.q[0] = 1;
    filter.beta = FLIGHT_BETA;
    GyroCalibration_Reset(&gyro_cal);
    config_ok = (uint8_t)calibration_valid();
    imu_ok = (uint8_t)IMU_Init();
    mag_present = (uint8_t)Magnetometer_Init();
    SEGGER_RTT_WriteString(0, "Bench attitude firmware; motor PWM disabled.\n");
    if (!config_ok)
        SEGGER_RTT_WriteString(0, "ERROR: invalid mapping/calibration matrix.\n");
    if (!imu_ok)
        SEGGER_RTT_WriteString(
            0, "ERROR: MPU6050 init/readback failed; DIAG/FIXED still available.\n");
    if (!mag_present)
        SEGGER_RTT_WriteString(0, "HMC5883L H43 not detected; MARG unavailable.\n");
    attitude_sem = OSSemCreate(0);
    telemetry_sem = OSSemCreate(0);
    if (!attitude_sem || !telemetry_sem)
        Error_Handler();
    error = OSTaskCreateExt(AttitudeTask, NULL, &attitude_stack[767], ATTITUDE_PRIO, ATTITUDE_PRIO,
                            attitude_stack, 768, NULL, OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
    if (error != OS_ERR_NONE)
        Error_Handler();
    OSTaskNameSet(ATTITUDE_PRIO, (INT8U *)"Attitude 200Hz", &error);
    if (error != OS_ERR_NONE)
        Error_Handler();
    error =
        OSTaskCreateExt(TelemetryTask, NULL, &telemetry_stack[767], TELEMETRY_PRIO, TELEMETRY_PRIO,
                        telemetry_stack, 768, NULL, OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
    if (error != OS_ERR_NONE)
        Error_Handler();
    OSTaskNameSet(TELEMETRY_PRIO, (INT8U *)"Bluetooth telemetry", &error);
    if (error != OS_ERR_NONE)
        Error_Handler();
    publish();
    __HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_UPDATE);
    if (HAL_TIM_Base_Start_IT(&htim4) != HAL_OK)
        Error_Handler();
}
