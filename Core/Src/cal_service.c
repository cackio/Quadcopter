#include "cal_service.h"
#include "timebase.h"
#include <math.h>
#include <string.h>

void CalMoments_Push(CalMoments *s, const ImuRawSample *raw)
{
    unsigned i;
    float value, delta, centered;
    ++s->count;
    for (i = 0; i < 7; ++i)
    {
        value = i < 3 ? raw->accel[i] : i < 6 ? raw->gyro[i - 3] : raw->temperature;
        if (s->count == 1)
            s->origin[i] = value;
        /* Center near zero: a 1g raw baseline (~16384) otherwise loses small
         * mean updates at float precision. Preserve the public mean in LSB. */
        centered = value - s->origin[i];
        delta = centered - s->centered[i];
        s->centered[i] += delta / (float)s->count;
        s->mean[i] = s->origin[i] + s->centered[i];
        s->m2[i] += delta * (centered - s->centered[i]);
    }
}
float CalMoments_Std(const CalMoments *s, unsigned i)
{
    return s->count > 1 && i < 7 ? sqrtf(fmaxf(0, s->m2[i] / (float)(s->count - 1))) : 0;
}
void CalService_Init(CalService *s)
{
    memset(s, 0, sizeof(*s));
}
static int running(const CalService *s)
{
    return s->state == CAL_WAIT || s->state == CAL_COLLECT;
}
int CalService_Command(CalService *s, const CalCommand *cmd, uint32_t now)
{
    if (cmd->op == CAL_STATUS)
        return cmd->id == s->id ? CAL_OK : CAL_STALE;
    if (cmd->op == CAL_CANCEL)
    {
        if (cmd->id != s->id)
            return CAL_STALE;
        if (running(s))
            s->state = CAL_CANCELLED;
        return CAL_OK;
    }
    if ((cmd->op != CAL_GYRO && cmd->op != CAL_STATIC) || cmd->samples != CAL_CAPTURE_SAMPLES ||
        (cmd->op == CAL_STATIC && cmd->face > 18) || (cmd->op == CAL_GYRO && cmd->face != 0) ||
        !cmd->id)
        return CAL_BAD_COMMAND;
    if (cmd->id == s->id)
        return cmd->op == s->op && cmd->face == s->face && cmd->samples == s->target
                   ? CAL_OK
                   : CAL_BAD_COMMAND;
    if (s->id && (int32_t)(cmd->id - s->id) <= 0)
        return CAL_STALE;
    if (running(s))
        return CAL_BUSY;
    s->id = cmd->id;
    s->op = cmd->op;
    s->face = cmd->face;
    s->target = cmd->samples;
    s->started_us = s->last_new_us = now;
    s->last_sequence = s->first_sequence = s->capture_us = s->duration_us = s->dt_bad = 0;
    s->have_sequence = 0;
    s->resets = 0;
    s->state = CAL_WAIT;
    s->reason = s->last_reason = CAL_OK;
    memset(&s->moments, 0, sizeof(s->moments));
    if (cmd->op == CAL_GYRO)
        s->gyro_ready = 0;
    return CAL_OK;
}
static void fail(CalService *s, uint8_t reason)
{
    s->state = CAL_FAILED;
    s->reason = reason;
}
static void restart(CalService *s, uint8_t reason)
{
    s->state = CAL_WAIT;
    s->last_reason = reason;
    if (s->resets != 65535)
        ++s->resets;
    memset(&s->moments, 0, sizeof(s->moments));
}
static int quiet(const CalMoments *m)
{
    unsigned i;
    for (i = 0; i < 3; ++i)
        if (CalMoments_Std(m, i) > CAL_ACCEL_STD_G * 16384 ||
            CalMoments_Std(m, i + 3) > CAL_GYRO_STD_DPS * 65.5f)
            return 0;
    return 1;
}
void CalService_Feed(CalService *s, const ImuRawSample *raw, int result, uint32_t sequence,
                     uint32_t now)
{
    unsigned i;
    float n = 0, v;
    uint32_t dt;
    if (!running(s))
        return;
    if (result == SENSOR_READ_ERROR)
    {
        fail(s, CAL_IO_ERROR);
        return;
    }
    if (Time_ElapsedUs(now, s->started_us) >= CAL_TIMEOUT_US)
    {
        fail(s, CAL_TIMEOUT);
        return;
    }
    if (result != SENSOR_NEW || !raw || (s->have_sequence && sequence == s->last_sequence))
    {
        if (Time_ElapsedUs(now, s->last_new_us) >= CAL_NO_SAMPLE_US)
            fail(s, CAL_NO_SAMPLE);
        return;
    }
    dt = Time_ElapsedUs(now, s->last_new_us);
    if (s->have_sequence && dt > 20000U)
    {
        fail(s, CAL_TIME_GAP);
        return;
    }
    s->last_new_us = now;
    s->last_sequence = sequence;
    s->have_sequence = 1;
    if (dt < 4500U || dt > 5500U)
        ++s->dt_bad;
    for (i = 0; i < 3; ++i)
    {
        v = (float)raw->accel[i] / 16384;
        n += v * v;
    }
    if (n < CAL_RAW_ACCEL_MIN_G * CAL_RAW_ACCEL_MIN_G ||
        n > CAL_RAW_ACCEL_MAX_G * CAL_RAW_ACCEL_MAX_G)
    {
        restart(s, CAL_ACCEL_RANGE);
        return;
    }
    for (i = 0; i < 3; ++i)
    {
        v = (float)raw->gyro[i] / 65.5f;
        if (fabsf(v - (s->gyro_ready ? s->bias_dps[i] : 0)) > 20 ||
            (s->moments.count >= 20 &&
             (fabsf((float)raw->gyro[i] - s->moments.mean[i + 3]) > CAL_GYRO_DELTA_DPS * 65.5f ||
              fabsf((float)raw->accel[i] - s->moments.mean[i]) > 0.05f * 16384)))
        {
            restart(s, CAL_MOTION);
            return;
        }
    }
    if (s->state == CAL_COLLECT && !s->moments.count)
    {
        s->first_sequence = sequence;
        s->capture_us = now;
    }
    CalMoments_Push(&s->moments, raw);
    if (s->state == CAL_WAIT && s->moments.count >= CAL_STABLE_SAMPLES)
    {
        if (!quiet(&s->moments))
        {
            restart(s, CAL_NOISE);
            return;
        }
        memset(&s->moments, 0, sizeof(s->moments));
        s->state = CAL_COLLECT;
    }
    else if (s->state == CAL_COLLECT && s->moments.count >= s->target)
    {
        if (!quiet(&s->moments))
        {
            restart(s, CAL_NOISE);
            return;
        }
        s->duration_us = Time_ElapsedUs(now, s->capture_us);
        s->state = CAL_DONE;
        if (s->op == CAL_GYRO)
        {
            for (i = 0; i < 3; ++i)
                s->bias_dps[i] = s->moments.mean[i + 3] / 65.5f;
            s->gyro_ready = 1;
        }
    }
}

/* Compatibility API used by normal attitude startup. Units: g and deg/s. */
void GyroCalibration_Reset(GyroCalibration *c)
{
    memset(c, 0, sizeof(*c));
}
int GyroCalibration_Push(GyroCalibration *c, const float accel[3], const float dps[3], uint32_t n)
{
    float v[6], norm = 0, delta;
    unsigned i;
    if (c->ready)
        return 1;
    for (i = 0; i < 3; ++i)
    {
        if (!isfinite(accel[i]) || !isfinite(dps[i]))
        {
            GyroCalibration_Reset(c);
            return 0;
        }
        norm += accel[i] * accel[i];
        v[i] = dps[i];
        v[i + 3] = accel[i];
    }
    if (n < 2 || norm < CAL_RAW_ACCEL_MIN_G * CAL_RAW_ACCEL_MIN_G ||
        norm > CAL_RAW_ACCEL_MAX_G * CAL_RAW_ACCEL_MAX_G)
    {
        GyroCalibration_Reset(c);
        return 0;
    }
    for (i = 0; i < 3; ++i)
        if (fabsf(dps[i]) > 20 ||
            (c->count >= 20 && fabsf(dps[i] - c->mean[i]) > CAL_GYRO_DELTA_DPS))
        {
            GyroCalibration_Reset(c);
            return 0;
        }
    ++c->count;
    for (i = 0; i < 6; ++i)
    {
        delta = v[i] - c->mean[i];
        c->mean[i] += delta / (float)c->count;
        c->m2[i] += delta * (v[i] - c->mean[i]);
    }
    if (c->count < n)
        return 0;
    for (i = 0; i < 6; ++i)
        if (c->m2[i] / (float)(c->count - 1) >
            (i < 3 ? CAL_GYRO_STD_DPS * CAL_GYRO_STD_DPS : CAL_ACCEL_STD_G * CAL_ACCEL_STD_G))
        {
            GyroCalibration_Reset(c);
            return 0;
        }
    for (i = 0; i < 3; ++i)
        c->bias_dps[i] = c->mean[i];
    c->ready = 1;
    return 1;
}
