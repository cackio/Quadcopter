#include "flight_app.h"
#include "SEGGER_RTT.h"
#include "ano_protocol.h"
#include "attitude.h"
#include "bsp.h"
#include "cal_wire.h"
#include "flight_config.h"
#include "imu_driver.h"
#include "sensor_calibration.h"
#include "sensor_data.h"
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
static CalService calibration;
static CalParser command_parser;
static CalCommand command_mailbox;
static volatile uint8_t command_pending, reply_pending;
static uint8_t reply_frame[96], rx_ring[256], rx_byte, quiet_slot;
static uint16_t reply_size;
static volatile uint16_t rx_read, rx_write;
static volatile uint32_t rx_overflow, rx_errors, rx_bytes;
static void handle_cal_command(void);
static void poll_commands(void);
static void transmit_frame(uint16_t size);
static float display_basis[4] = FLIGHT_DISPLAY_BASIS_Q;

static int calibration_valid(void)
{
    return isfinite(FLIGHT_BETA) && FLIGHT_BETA >= 0 && Sensor_ConfigValid() &&
           Attitude_Normalize(display_basis);
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
    uint16_t divisor =
        (FLIGHT_APP_MODE == FLIGHT_APP_SENSOR_CAL || FLIGHT_TELEMETRY_MODE == FLIGHT_TELEM_DIAG)
            ? FLIGHT_IMU_HZ / FLIGHT_DIAG_HZ
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
    Sensor_Mag(raw, working.mag_body);
}
static void handle_cal_command(void)
{
    CalCommand cmd;
    OS_CPU_SR cpu_sr = 0;
    uint8_t data[84];
    float values[18];
    unsigned i, n;
    int reason;
    const SensorDiagnostic *d;
    if (!command_pending || reply_pending)
        return;
    OS_ENTER_CRITICAL();
    cmd = command_mailbox;
    command_pending = 0;
    OS_EXIT_CRITICAL();
    if (cmd.op == CAL_INFO)
    {
        CalWire_Header(data, &cmd, CAL_DONE, CAL_OK);
        if (cmd.face == 0)
        {
            data[SCAL_INFO_OFF_APP_MODE] = FLIGHT_APP_MODE;
            data[SCAL_INFO_OFF_IMU_OK] = imu_ok;
            data[SCAL_INFO_OFF_MAG_PRESENT] = mag_present;
            data[SCAL_INFO_OFF_CONFIG_OK] = config_ok;
            ANO_Put32(data + SCAL_INFO_OFF_BAUD, FLIGHT_UART_BAUD);
            ANO_Put32(data + SCAL_INFO_OFF_IMU_HZ, FLIGHT_IMU_HZ);
            ANO_Put32(data + SCAL_INFO_OFF_CAL_FLAGS, CAL_ACCEL_VALID | (CAL_MAG_VALID << 1) |
                                                          (FLIGHT_IMU_AXES_CONFIRMED << 2) |
                                                          (FLIGHT_MAG_AXES_CONFIRMED << 3));
            ANO_Put32(data + SCAL_INFO_OFF_IMU_ERRORS, Sensor_Diagnostic(0)->read_errors);
            ANO_Put32(data + SCAL_INFO_OFF_MAG_ERRORS, Sensor_Diagnostic(1)->read_errors);
            ANO_Put32(data + SCAL_INFO_OFF_RX_OVERFLOW, rx_overflow);
            ANO_Put32(data + SCAL_INFO_OFF_BAD_COMMANDS, command_parser.bad_frames);
            ANO_Put32(data + SCAL_INFO_OFF_RX_ERRORS, rx_errors);
            ANO_Put32(data + SCAL_INFO_OFF_UPTIME_MS, HAL_GetTick());
            ANO_Put32(data + SCAL_INFO_OFF_DT_TOTAL, working.dt_total);
            ANO_Put32(data + SCAL_INFO_OFF_DT_IN_BAND, working.dt_in_band);
            ANO_Put32(data + SCAL_INFO_OFF_EXEC_MAX_US, working.exec_max_us);
            ANO_Put32(data + SCAL_INFO_OFF_MISSED, release_missed);
            ANO_Put32(data + SCAL_INFO_OFF_LAST_JOB_ID, calibration.id);
            ANO_Put32(data + SCAL_INFO_OFF_FIRMWARE_MAGIC, 0x53434131U);
            ANO_Put32(data + SCAL_INFO_OFF_RX_BYTES, rx_bytes);
            ANO_Put32(data + SCAL_INFO_OFF_DIAGNOSTIC_VERSION, 1);
        }
        else if (cmd.face == 1 || cmd.face == 2)
        {
            d = Sensor_Diagnostic(cmd.face - 1);
            data[SCAL_DRIVER_OFF_ADDRESS] = d->address;
            memcpy(data + SCAL_DRIVER_OFF_ID, d->id, 3);
            data[SCAL_DRIVER_OFF_STAGE] = d->stage;
            data[SCAL_DRIVER_OFF_REGISTER] = d->reg;
            data[SCAL_DRIVER_OFF_EXPECTED] = d->expected;
            data[SCAL_DRIVER_OFF_ACTUAL] = d->actual;
            data[SCAL_DRIVER_OFF_HAL_STATUS] = d->status;
            data[SCAL_DRIVER_OFF_INITIALIZED] = d->initialized;
            ANO_Put32(data + SCAL_DRIVER_OFF_HAL_ERROR, d->hal_error);
            ANO_Put32(data + SCAL_DRIVER_OFF_READ_ERRORS, d->read_errors);
            ANO_Put32(data + SCAL_DRIVER_OFF_OVERFLOW, d->overflow);
            ANO_Put32(data + SCAL_DRIVER_OFF_NOT_READY, d->not_ready);
            ANO_Put32(data + SCAL_DRIVER_OFF_NEW_SAMPLES, d->new_samples);
        }
        else if (cmd.face >= 3 && cmd.face <= 5)
        {
            n = Sensor_Coefficients(cmd.face, values);
            data[SCAL_COEFF_OFF_COUNT] = (uint8_t)n;
            for (i = 0; i < n; ++i)
                CalWire_Float(data + SCAL_COEFF_OFF_VALUES + 4 * i, values[i]);
        }
        else if (cmd.face == 6)
        {
            for (i = 0; i < 3; ++i)
                CalWire_Float(data + SCAL_COEFF_OFF_VALUES + 4 * i,
                              FLIGHT_APP_MODE == FLIGHT_APP_SENSOR_CAL ? calibration.bias_dps[i]
                                                                       : gyro_cal.bias_dps[i]);
            data[SCAL_GYRO_OFF_READY] =
                FLIGHT_APP_MODE == FLIGHT_APP_SENSOR_CAL ? calibration.gyro_ready : gyro_cal.ready;
        }
        else
        {
            data[3] = CAL_FAILED;
            data[8] = CAL_BAD_COMMAND;
        }
        reply_size = ANO_Pack(SCAL_REPLY_ID, data, 84, reply_frame, 96);
    }
    else
    {
        reason = FLIGHT_APP_MODE == FLIGHT_APP_SENSOR_CAL
                     ? CalService_Command(&calibration, &cmd, BSP_TimeUs())
                     : CAL_UNAVAILABLE;
        if (reason == CAL_OK)
            reply_size = CalWire_Result(&calibration, reply_frame);
        else
        {
            CalWire_Header(data, &cmd, CAL_FAILED, (uint8_t)reason);
            reply_size = ANO_Pack(SCAL_REPLY_ID, data, 84, reply_frame, 96);
        }
    }
    __DMB();
    reply_pending = 1;
}
static void poll_commands(void)
{
    static uint32_t seen_overflow;
    CalCommand cmd;
    OS_CPU_SR cpu_sr = 0;
    unsigned budget = 256;
    uint8_t value;
    if (seen_overflow != rx_overflow)
    {
        OS_ENTER_CRITICAL();
        rx_read = rx_write;
        seen_overflow = rx_overflow;
        OS_EXIT_CRITICAL();
        command_parser.used = 0;
    }
    while (budget-- && rx_read != rx_write && !command_pending && !reply_pending)
    {
        value = rx_ring[rx_read];
        rx_read = (uint16_t)((rx_read + 1U) & 255U);
        if (CalParser_Feed(&command_parser, value, &cmd))
        {
            OS_ENTER_CRITICAL();
            command_mailbox = cmd;
            command_pending = 1;
            OS_EXIT_CRITICAL();
        }
    }
}
static void transmit_frame(uint16_t size)
{
    tx_started_us = BSP_TimeUs();
    tx_deadline_us =
        ((uint32_t)size * 10000000U + FLIGHT_UART_BAUD - 1U) / FLIGHT_UART_BAUD + 20000U;
    tx_busy = 1;
    if (HAL_UART_Transmit_IT(&huart1, tx_buffer, size) != HAL_OK)
    {
        tx_busy = 0;
        ++tx_dropped;
    }
}
static void process_sample(void)
{
    ImuRawSample raw;
    float native_acc[3], native_dps[3], norm;
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
        if (FLIGHT_APP_MODE == FLIGHT_APP_SENSOR_CAL)
            CalService_Feed(&calibration, NULL, result, working.sequence, now);
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
        native_dps[i] = (float)raw.gyro[i] / 65.5f;
    }
    if (FLIGHT_APP_MODE == FLIGHT_APP_SENSOR_CAL)
    {
        CalService_Feed(&calibration, &raw, result, working.sequence, now);
        working.cal_samples = (uint16_t)calibration.moments.count;
        if (calibration.gyro_ready)
            working.flags |= STATUS_GYRO_CALIBRATED;
        return; /* Raw sampling never calls Madgwick or depends on fitted config. */
    }
    if (!config_ok)
        return;
    Sensor_Accel(raw.accel, native_acc, working.accel_g);
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
    }
    Sensor_Gyro(raw.gyro, gyro_cal.bias_dps, working.gyro_rad_s);
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
        handle_cal_command();
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
    if (FLIGHT_APP_MODE == FLIGHT_APP_SENSOR_CAL || FLIGHT_TELEMETRY_MODE == FLIGHT_TELEM_DIAG)
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
    transmit_frame(size);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uart)
{
    if (uart->Instance == USART1)
        tx_busy = 0;
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart)
{
    uint16_t next;
    if (uart->Instance != USART1)
        return;
    ++rx_bytes;
    next = (uint16_t)((rx_write + 1U) & 255U);
    if (next == rx_read)
        ++rx_overflow;
    else
    {
        rx_ring[rx_write] = rx_byte;
        __DMB();
        rx_write = next;
    }
    (void)HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}
/* Do not free a still-running TX buffer on an unrelated RX line error. */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{
    if (uart->Instance == USART1)
    {
        ++rx_errors;
        if (uart->gState == HAL_UART_STATE_READY)
            tx_busy = 0;
        /* A blocking RX error ends RX in HAL. RX state is independent of TX. */
        if (uart->RxState == HAL_UART_STATE_READY)
            (void)HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}
static void telemetry_step(FlightSnapshot *s)
{
    OS_CPU_SR cpu_sr = 0;
    poll_commands();
    if (huart1.RxState == HAL_UART_STATE_READY)
        (void)HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    Flight_GetSnapshot(s);
    if (reply_pending && !tx_busy)
    {
        OS_ENTER_CRITICAL();
        memcpy(tx_buffer, reply_frame, reply_size);
        {
            uint16_t size = reply_size;
            reply_pending = 0;
            OS_EXIT_CRITICAL();
            transmit_frame(size);
        }
        quiet_slot = 1;
    }
    else if (quiet_slot)
        quiet_slot = 0;
    else
        send_snapshot(s);
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
        (FLIGHT_APP_MODE == FLIGHT_APP_SENSOR_CAL || FLIGHT_TELEMETRY_MODE == FLIGHT_TELEM_DIAG)
            ? FLIGHT_DIAG_HZ
            : FLIGHT_ANO_HZ;
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
        telemetry_step(&s);
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
    CalService_Init(&calibration);
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
    (void)HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    __HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_UPDATE);
    if (HAL_TIM_Base_Start_IT(&htim4) != HAL_OK)
        Error_Handler();
}
