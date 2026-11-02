#ifndef FLIGHT_CONFIG_H
#define FLIGHT_CONFIG_H

/* Bench firmware: motor outputs are deliberately never started. */
#define FLIGHT_FUSION_GYRO 0
#define FLIGHT_FUSION_IMU 1
#define FLIGHT_FUSION_MARG 2
#define FLIGHT_TELEM_ANO 0
#define FLIGHT_TELEM_DIAG 1
#define FLIGHT_TELEM_FIXED 2

#ifndef FLIGHT_UART_BAUD
#define FLIGHT_UART_BAUD 9600U
#endif
#ifndef FLIGHT_FUSION_MODE
#define FLIGHT_FUSION_MODE FLIGHT_FUSION_IMU
#endif
#ifndef FLIGHT_TELEMETRY_MODE
#define FLIGHT_TELEMETRY_MODE FLIGHT_TELEM_ANO
#endif
#define FLIGHT_IMU_HZ 200U
#define FLIGHT_PERIOD_US (1000000U / FLIGHT_IMU_HZ)
#define FLIGHT_DIAG_HZ 10U
#if FLIGHT_UART_BAUD >= 115200U
#define FLIGHT_ANO_HZ 50U
#define FLIGHT_SEND_QUATERNION 1
#else
#define FLIGHT_ANO_HZ 20U
#define FLIGHT_SEND_QUATERNION 0
#endif
#ifndef FLIGHT_BETA
#define FLIGHT_BETA 0.05f
#endif
#define FLIGHT_GYRO_WARMUP_SAMPLES 200U
#define FLIGHT_GYRO_CAL_SAMPLES 1000U
#define FLIGHT_MAG_MAX_AGE_US 100000U
#define FLIGHT_MAX_DT_US 20000U
#define FLIGHT_I2C_TIMEOUT_MS 3U
#define FLIGHT_ACCEL_MIN_G 0.85f
#define FLIGHT_ACCEL_MAX_G 1.15f
/* Change these ONLY after checking actual sensor-to-body directions. */
#ifndef FLIGHT_IMU_AXES_CONFIRMED
#define FLIGHT_IMU_AXES_CONFIRMED 0
#endif
#ifndef FLIGHT_MAG_AXES_CONFIRMED
#define FLIGHT_MAG_AXES_CONFIRMED 0
#endif
/* Frame conjugation q_display = r * q * conjugate(r), identity by default. */
#define FLIGHT_DISPLAY_BASIS_Q {1.0f, 0.0f, 0.0f, 0.0f}
/* FIXED mode bypasses sensor validity; use 0/30 deg one axis at a time. */
#ifndef FLIGHT_FIXED_ROLL_DEG
#define FLIGHT_FIXED_ROLL_DEG 0.0f
#endif
#ifndef FLIGHT_FIXED_PITCH_DEG
#define FLIGHT_FIXED_PITCH_DEG 0.0f
#endif
#ifndef FLIGHT_FIXED_YAW_DEG
#define FLIGHT_FIXED_YAW_DEG 0.0f
#endif

#if FLIGHT_FUSION_MODE < 0 || FLIGHT_FUSION_MODE > 2
#error Invalid FLIGHT_FUSION_MODE
#endif
#if FLIGHT_TELEMETRY_MODE < 0 || FLIGHT_TELEMETRY_MODE > 2
#error Invalid FLIGHT_TELEMETRY_MODE
#endif
#if (FLIGHT_IMU_HZ % FLIGHT_ANO_HZ) || (FLIGHT_IMU_HZ % FLIGHT_DIAG_HZ)
#error Telemetry rates must divide the IMU timer rate
#endif
/* 8N1: keep serial utilization below 75 percent. DIAG is 67 bytes. */
#if FLIGHT_TELEMETRY_MODE == FLIGHT_TELEM_DIAG
#if (67U * FLIGHT_DIAG_HZ * 10U * 4U) > (FLIGHT_UART_BAUD * 3U)
#error Diagnostic telemetry exceeds serial budget
#endif
#elif ((13U + 15U * FLIGHT_SEND_QUATERNION) * FLIGHT_ANO_HZ * 10U * 4U) > (FLIGHT_UART_BAUD * 3U)
#error ANO telemetry exceeds serial budget
#endif
#endif
