#ifndef CAL_SERVICE_H
#define CAL_SERVICE_H
#include "imu_driver.h"
#include <stdint.h>

/* Raw/native-frame statistics. Channel order ax,ay,az,gx,gy,gz,temp;
 * mean/std are LSB, std uses N-1. No fitted coefficients enter sampling. */
#define CAL_STABLE_SAMPLES 200U
#define CAL_CAPTURE_SAMPLES 1000U
#define CAL_TIMEOUT_US 30000000U
#define CAL_NO_SAMPLE_US 2000000U
#define CAL_GYRO_STD_DPS 0.2f
#define CAL_ACCEL_STD_G 0.02f
#define CAL_RAW_ACCEL_MIN_G 0.8f
#define CAL_RAW_ACCEL_MAX_G 1.2f
#define CAL_GYRO_DELTA_DPS 1.5f

typedef struct
{
    uint32_t count;
    float mean[7], m2[7], origin[7], centered[7];
} CalMoments;
typedef struct
{
    uint32_t count;
    float mean[6], m2[6], bias_dps[3];
    uint8_t ready;
} GyroCalibration;

enum
{
    CAL_IDLE,
    CAL_WAIT,
    CAL_COLLECT,
    CAL_DONE,
    CAL_FAILED,
    CAL_CANCELLED
};
enum
{
    CAL_INFO = 1,
    CAL_GYRO = 2,
    CAL_STATIC = 3,
    CAL_STATUS = 4,
    CAL_CANCEL = 5
};
enum
{
    CAL_OK,
    CAL_MOTION,
    CAL_ACCEL_RANGE,
    CAL_NOISE,
    CAL_IO_ERROR,
    CAL_NO_SAMPLE,
    CAL_TIMEOUT,
    CAL_TIME_GAP,
    CAL_BAD_COMMAND,
    CAL_BUSY,
    CAL_STALE,
    CAL_UNAVAILABLE
};
typedef struct
{
    uint32_t id;
    uint8_t op, face;
    uint16_t samples;
} CalCommand;
typedef struct
{
    uint32_t id, started_us, last_new_us, last_sequence, first_sequence, capture_us, duration_us;
    uint32_t dt_bad;
    uint16_t target, resets;
    uint8_t op, face, state, reason, last_reason, have_sequence, gyro_ready;
    float bias_dps[3];
    CalMoments moments;
} CalService;

void CalMoments_Push(CalMoments *s, const ImuRawSample *raw);
float CalMoments_Std(const CalMoments *s, unsigned channel);
void CalService_Init(CalService *s);
/* Rejected commands do not modify the active job. Same ID/content is idempotent.
 * IDs increase modulo 2^32; INFO exposes last accepted ID for reconnects. */
int CalService_Command(CalService *s, const CalCommand *cmd, uint32_t now);
void CalService_Feed(CalService *s, const ImuRawSample *raw, int result, uint32_t sequence,
                     uint32_t now);
void GyroCalibration_Reset(GyroCalibration *c);
int GyroCalibration_Push(GyroCalibration *c, const float accel[3], const float dps[3], uint32_t n);
#endif
