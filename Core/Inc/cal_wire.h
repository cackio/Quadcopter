#ifndef CAL_WIRE_H
#define CAL_WIRE_H
#include "cal_service.h"
#include <stddef.h>
/* Shared with tools/protocol.py. Float fields are IEEE754 binary32 LE, packed
 * explicitly. F1 v1 is unchanged. F3 fixed payload=84, complete frame=90. */
#define SCAL_VERSION 1
#define SCAL_CMD_ID 0xF2
#define SCAL_REPLY_ID 0xF3
#define SCAL_CMD_SIZE 12
#define SCAL_REPLY_SIZE 84
#define SCAL_OFF_VERSION 0
#define SCAL_OFF_OP 1
#define SCAL_OFF_FACE 2
#define SCAL_OFF_STATE 3
#define SCAL_OFF_ID 4
#define SCAL_OFF_REASON 8
#define SCAL_OFF_LAST_REASON 9
#define SCAL_OFF_COUNT 10
#define SCAL_OFF_TARGET 12
#define SCAL_OFF_RESETS 14
#define SCAL_OFF_FIRST_SEQ 16
#define SCAL_OFF_LAST_SEQ 20
#define SCAL_OFF_DURATION_US 24
#define SCAL_OFF_MEAN 28
#define SCAL_OFF_STD 52
#define SCAL_OFF_TEMPERATURE_C 76
#define SCAL_OFF_DT_BAD 80
#define SCAL_CMD_OFF_SAMPLES 8
#define SCAL_INFO_OFF_APP_MODE 12
#define SCAL_INFO_OFF_IMU_OK 13
#define SCAL_INFO_OFF_MAG_PRESENT 14
#define SCAL_INFO_OFF_CONFIG_OK 15
#define SCAL_INFO_OFF_BAUD 16
#define SCAL_INFO_OFF_IMU_HZ 20
#define SCAL_INFO_OFF_CAL_FLAGS 24
#define SCAL_INFO_OFF_IMU_ERRORS 28
#define SCAL_INFO_OFF_MAG_ERRORS 32
#define SCAL_INFO_OFF_RX_OVERFLOW 36
#define SCAL_INFO_OFF_BAD_COMMANDS 40
#define SCAL_INFO_OFF_RX_ERRORS 44
#define SCAL_INFO_OFF_UPTIME_MS 48
#define SCAL_INFO_OFF_DT_TOTAL 52
#define SCAL_INFO_OFF_DT_IN_BAND 56
#define SCAL_INFO_OFF_EXEC_MAX_US 60
#define SCAL_INFO_OFF_MISSED 64
#define SCAL_INFO_OFF_LAST_JOB_ID 68
#define SCAL_INFO_OFF_FIRMWARE_MAGIC 72
#define SCAL_INFO_OFF_RX_BYTES 76
#define SCAL_INFO_OFF_DIAGNOSTIC_VERSION 80
#define SCAL_DRIVER_OFF_ADDRESS 12
#define SCAL_DRIVER_OFF_ID 13
#define SCAL_DRIVER_OFF_STAGE 16
#define SCAL_DRIVER_OFF_REGISTER 17
#define SCAL_DRIVER_OFF_EXPECTED 18
#define SCAL_DRIVER_OFF_ACTUAL 19
#define SCAL_DRIVER_OFF_HAL_STATUS 20
#define SCAL_DRIVER_OFF_INITIALIZED 21
#define SCAL_DRIVER_OFF_HAL_ERROR 24
#define SCAL_DRIVER_OFF_READ_ERRORS 28
#define SCAL_DRIVER_OFF_OVERFLOW 32
#define SCAL_DRIVER_OFF_NOT_READY 36
#define SCAL_DRIVER_OFF_NEW_SAMPLES 40
#define SCAL_COEFF_OFF_COUNT 10
#define SCAL_COEFF_OFF_VALUES 12
#define SCAL_GYRO_OFF_BIAS_DPS 12
#define SCAL_GYRO_OFF_READY 24
typedef struct
{
    uint8_t bytes[18];
    unsigned used;
    uint32_t bad_frames;
} CalParser;
int CalParser_Feed(CalParser *p, uint8_t byte, CalCommand *cmd);
uint16_t CalWire_Result(const CalService *s, uint8_t out[96]);
void CalWire_Header(uint8_t data[84], const CalCommand *cmd, uint8_t state, uint8_t reason);
void CalWire_Float(uint8_t *out, float value);
#endif
