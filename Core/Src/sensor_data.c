#include "sensor_data.h"
#include "attitude.h"
#include "sensor_calibration.h"
#include <math.h>
#include <string.h>
static const float bias[3] = CAL_ACCEL_BIAS_G, scale[3] = CAL_ACCEL_SCALE;
static const float mbias[3] = CAL_MAG_BIAS_RAW, soft[3][3] = CAL_MAG_MATRIX;
static const float ir[3][3] = CAL_IMU_TO_BODY, mr[3][3] = CAL_MAG_TO_BODY;
int Sensor_ConfigValid(void)
{
    float a, b, c, d, e, f;
    unsigned i, j;
    if (!Attitude_MatrixValid(ir) || !Attitude_MatrixValid(mr))
        return 0;
    for (i = 0; i < 3; ++i)
    {
        if (!isfinite(bias[i]) || !isfinite(scale[i]) || scale[i] <= 0 || !isfinite(mbias[i]))
            return 0;
        for (j = 0; j < 3; ++j)
            if (!isfinite(soft[i][j]) || fabsf(soft[i][j] - soft[j][i]) > 1e-5f)
                return 0;
    }
    a = soft[0][0];
    if (a <= 0)
        return 0;
    a = sqrtf(a);
    b = soft[1][0] / a;
    c = soft[2][0] / a;
    d = soft[1][1] - b * b;
    if (d <= 0)
        return 0;
    d = sqrtf(d);
    e = (soft[2][1] - b * c) / d;
    f = soft[2][2] - c * c - e * e;
    return isfinite(f) && f > 0;
}
void Sensor_Accel(const int16_t raw[3], float native[3], float body[3])
{
    unsigned i;
    for (i = 0; i < 3; ++i)
        native[i] = ((float)raw[i] / 16384 - bias[i]) * scale[i];
    Attitude_Rotate(ir, native, body);
}
void Sensor_Gyro(const int16_t raw[3], const float bias_dps[3], float body[3])
{
    float native[3];
    unsigned i;
    for (i = 0; i < 3; ++i)
        native[i] = ((float)raw[i] / 65.5f - bias_dps[i]) * 0.017453292519943295f;
    Attitude_Rotate(ir, native, body);
}
void Sensor_Mag(const int16_t raw[3], float body[3])
{
    float center[3], corrected[3];
    unsigned i;
    for (i = 0; i < 3; ++i)
        center[i] = (float)raw[i] - mbias[i];
    Attitude_Rotate(soft, center, corrected);
    Attitude_Rotate(mr, corrected, body);
}
unsigned Sensor_Coefficients(unsigned page, float out[18])
{
    if (page == 3)
    {
        memcpy(out, bias, 12);
        memcpy(out + 3, scale, 12);
        return 6;
    }
    if (page == 4)
    {
        memcpy(out, mbias, 12);
        memcpy(out + 3, soft, 36);
        return 12;
    }
    if (page == 5)
    {
        memcpy(out, ir, 36);
        memcpy(out + 9, mr, 36);
        return 18;
    }
    return 0;
}
