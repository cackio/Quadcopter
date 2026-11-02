#include "ano_protocol.h"
#include <math.h>
#include <string.h>
void ANO_Put16(uint8_t *p, uint16_t x)
{
    p[0] = (uint8_t)x;
    p[1] = (uint8_t)(x >> 8);
}
void ANO_Put32(uint8_t *p, uint32_t x)
{
    ANO_Put16(p, (uint16_t)x);
    ANO_Put16(p + 2, (uint16_t)(x >> 16));
}
int16_t ANO_Scale(float x, float scale)
{
    float v = x * scale;
    if (!isfinite(v))
        return 0;
    if (v > 32767)
        return 32767;
    if (v < -32768)
        return -32768;
    return (int16_t)lroundf(v);
}
uint16_t ANO_Pack(uint8_t id, const uint8_t *data, uint8_t len, uint8_t *out, size_t capacity)
{
    uint16_t i, n = (uint16_t)len + 4;
    uint8_t sc = 0, ac = 0;
    if (!out || capacity < (size_t)len + 6 || (len && !data))
        return 0;
    out[0] = 0xAA;
    out[1] = 0xFF;
    out[2] = id;
    out[3] = len;
    if (len)
        memcpy(out + 4, data, len);
    for (i = 0; i < n; ++i)
    {
        sc = (uint8_t)(sc + out[i]);
        ac = (uint8_t)(ac + sc);
    }
    out[n++] = sc;
    out[n++] = ac;
    return n;
}
uint16_t ANO_PackEuler(const float e[3], uint8_t *out, size_t cap)
{
    uint8_t d[7] = {0};
    unsigned i;
    for (i = 0; i < 3; ++i)
    {
        if (!isfinite(e[i]))
            return 0;
        ANO_Put16(d + 2 * i, (uint16_t)ANO_Scale(e[i], 100));
    }
    return ANO_Pack(3, d, 7, out, cap);
}
uint16_t ANO_PackQuaternion(const float q[4], uint8_t *out, size_t cap)
{
    uint8_t d[9] = {0};
    unsigned i;
    for (i = 0; i < 4; ++i)
    {
        if (!isfinite(q[i]))
            return 0;
        ANO_Put16(d + 2 * i, (uint16_t)ANO_Scale(q[i], 10000));
    }
    return ANO_Pack(4, d, 9, out, cap);
}
static uint16_t saturate_u16(uint32_t v)
{
    return v > 65535U ? 65535U : (uint16_t)v;
}
uint16_t ANO_PackDiagnostic(const FlightSnapshot *s, uint8_t *out, size_t cap)
{
    uint8_t d[DIAG_PAYLOAD_SIZE] = {0};
    unsigned i;
    d[DIAG_OFF_VERSION] = DIAG_VERSION;
    ANO_Put16(d + DIAG_OFF_FLAGS, s->flags);
    ANO_Put32(d + DIAG_OFF_SEQUENCE, s->sequence);
    ANO_Put32(d + DIAG_OFF_TIMESTAMP_US, s->timestamp_us);
    ANO_Put16(d + DIAG_OFF_DT_US, saturate_u16(s->dt_us));
    ANO_Put16(d + DIAG_OFF_EXEC_US, saturate_u16(s->exec_us));
    for (i = 0; i < 6; ++i)
        ANO_Put16(d + DIAG_OFF_IMU_RAW + 2 * i, (uint16_t)s->imu_raw[i]);
    for (i = 0; i < 3; ++i)
        ANO_Put16(d + DIAG_OFF_MAG_RAW + 2 * i, (uint16_t)s->mag_raw[i]);
    ANO_Put32(d + DIAG_OFF_MAG_SEQUENCE, s->mag_sequence);
    ANO_Put32(d + DIAG_OFF_MAG_AGE_US, s->mag_age_us);
    for (i = 0; i < 4; ++i)
        ANO_Put16(d + DIAG_OFF_QUATERNION + 2 * i, (uint16_t)ANO_Scale(s->q[i], 10000));
    ANO_Put32(d + DIAG_OFF_RELEASE_MISSED, s->release_missed);
    ANO_Put32(d + DIAG_OFF_I2C_ERRORS, s->i2c_errors);
    ANO_Put16(d + DIAG_OFF_CAL_SAMPLES, s->cal_samples);
    ANO_Put16(d + DIAG_OFF_TX_DROPPED, saturate_u16(s->tx_dropped));
    return ANO_Pack(DIAG_FRAME_ID, d, sizeof(d), out, cap);
}
