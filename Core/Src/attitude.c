#include "attitude.h"
#include <math.h>
#include <string.h>

#define RAD_TO_DEG 57.29577951308232f
#define DEG_TO_RAD 0.017453292519943295f
static int finite_vec(const float *v, unsigned n)
{
    unsigned i;
    for (i = 0; i < n; ++i)
        if (!isfinite(v[i]))
            return 0;
    return 1;
}
static float norm3(const float *v)
{
    return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}
static int unit3(const float *v, float *u)
{
    float n;
    unsigned i;
    if (!finite_vec(v, 3))
        return 0;
    n = norm3(v);
    if (!isfinite(n) || n < 1.0e-8f)
        return 0;
    for (i = 0; i < 3; ++i)
        u[i] = v[i] / n;
    return 1;
}
int Attitude_Normalize(float q[4])
{
    float n;
    unsigned i;
    if (!finite_vec(q, 4))
        return 0;
    n = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (!isfinite(n) || n < 1.0e-8f)
        return 0;
    for (i = 0; i < 4; ++i)
        q[i] /= n;
    return 1;
}
int Attitude_MatrixValid(const float r[3][3])
{
    unsigned i, j, k;
    float d, det;
    for (i = 0; i < 3; ++i)
    {
        if (!finite_vec(r[i], 3))
            return 0;
        for (j = 0; j < 3; ++j)
        {
            d = 0;
            for (k = 0; k < 3; ++k)
                d += r[i][k] * r[j][k];
            if (fabsf(d - (i == j ? 1.0f : 0.0f)) > 1.0e-3f)
                return 0;
        }
    }
    det = r[0][0] * (r[1][1] * r[2][2] - r[1][2] * r[2][1]) -
          r[0][1] * (r[1][0] * r[2][2] - r[1][2] * r[2][0]) +
          r[0][2] * (r[1][0] * r[2][1] - r[1][1] * r[2][0]);
    return fabsf(det - 1.0f) < 1.0e-3f;
}
void Attitude_Rotate(const float r[3][3], const float in[3], float out[3])
{
    float t[3];
    unsigned i;
    for (i = 0; i < 3; ++i)
        t[i] = r[i][0] * in[0] + r[i][1] * in[1] + r[i][2] * in[2];
    memcpy(out, t, sizeof(t));
}
static void multiply(const float a[4], const float b[4], float out[4])
{
    float t[4] = {a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3],
                  a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2],
                  a[0] * b[2] - a[1] * b[3] + a[2] * b[0] + a[3] * b[1],
                  a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0]};
    memcpy(out, t, sizeof(t));
}
void Attitude_FromEuler(float r, float p, float y, float q[4])
{
    float cr = cosf(r * DEG_TO_RAD * 0.5f), sr = sinf(r * DEG_TO_RAD * 0.5f);
    float cp = cosf(p * DEG_TO_RAD * 0.5f), sp = sinf(p * DEG_TO_RAD * 0.5f);
    float cy = cosf(y * DEG_TO_RAD * 0.5f), sy = sinf(y * DEG_TO_RAD * 0.5f);
    q[0] = cr * cp * cy + sr * sp * sy;
    q[1] = sr * cp * cy - cr * sp * sy;
    q[2] = cr * sp * cy + sr * cp * sy;
    q[3] = cr * cp * sy - sr * sp * cy;
}
void Attitude_ToEuler(const float q[4], float degrees[3])
{
    float w = q[0], x = q[1], y = q[2], z = q[3], s = 2.0f * (w * y - z * x);
    if (s > 1)
        s = 1;
    if (s < -1)
        s = -1;
    degrees[0] = atan2f(2 * (w * x + y * z), 1 - 2 * (x * x + y * y)) * RAD_TO_DEG;
    degrees[1] = asinf(s) * RAD_TO_DEG;
    degrees[2] = atan2f(2 * (w * z + x * y), 1 - 2 * (y * y + z * z)) * RAD_TO_DEG;
}
void Attitude_Display(const float q[4], const float basis[4], float out[4])
{
    float c[4] = {basis[0], -basis[1], -basis[2], -basis[3]}, t[4];
    multiply(basis, q, t);
    multiply(t, c, out);
    (void)Attitude_Normalize(out);
}
static void rotation(const float q[4], float r[3][3])
{
    float w = q[0], x = q[1], y = q[2], z = q[3];
    r[0][0] = 1 - 2 * (y * y + z * z);
    r[0][1] = 2 * (x * y - w * z);
    r[0][2] = 2 * (x * z + w * y);
    r[1][0] = 2 * (x * y + w * z);
    r[1][1] = 1 - 2 * (x * x + z * z);
    r[1][2] = 2 * (y * z - w * x);
    r[2][0] = 2 * (x * z - w * y);
    r[2][1] = 2 * (y * z + w * x);
    r[2][2] = 1 - 2 * (x * x + y * y);
}
int Attitude_Align(AttitudeFilter *f, const float accel[3], const float *mag)
{
    float a[3], m[3], r[3][3], earth[3], e[3], q[4];
    if (!unit3(accel, a))
        return 0;
    e[0] = atan2f(a[1], a[2]) * RAD_TO_DEG;
    e[1] = atan2f(-a[0], sqrtf(a[1] * a[1] + a[2] * a[2])) * RAD_TO_DEG;
    e[2] = 0;
    Attitude_FromEuler(e[0], e[1], 0, q);
    if (mag && unit3(mag, m))
    {
        rotation(q, r);
        Attitude_Rotate(r, m, earth);
        if (earth[0] * earth[0] + earth[1] * earth[1] > 1.0e-6f)
            e[2] = -atan2f(earth[1], earth[0]) * RAD_TO_DEG;
    }
    Attitude_FromEuler(e[0], e[1], e[2], f->q);
    return Attitude_Normalize(f->q);
}
/* J^T f for predicted body vector R(q)^T * earth, with earth held
 * constant in the gradient step. This is Madgwick's normalized gradient
 * descent, expressed as a reusable vector residual rather than expanded
 * six/nine-axis formulae. q maps body -> magnetic-north/west/up earth. */
static void gradient(const float q[4], const float v[3], const float measured[3], float s[4])
{
    float w = q[0], x = q[1], y = q[2], z = q[3], a = v[0], b = v[1], c = v[2];
    float ex = (1 - 2 * y * y - 2 * z * z) * a + 2 * (x * y + w * z) * b + 2 * (x * z - w * y) * c -
               measured[0];
    float ey = 2 * (x * y - w * z) * a + (1 - 2 * x * x - 2 * z * z) * b + 2 * (y * z + w * x) * c -
               measured[1];
    float ez = 2 * (x * z + w * y) * a + 2 * (y * z - w * x) * b + (1 - 2 * x * x - 2 * y * y) * c -
               measured[2];
    s[0] +=
        (2 * z * b - 2 * y * c) * ex + (-2 * z * a + 2 * x * c) * ey + (2 * y * a - 2 * x * b) * ez;
    s[1] += (2 * y * b + 2 * z * c) * ex + (2 * y * a - 4 * x * b + 2 * w * c) * ey +
            (2 * z * a - 2 * w * b - 4 * x * c) * ez;
    s[2] += (-4 * y * a + 2 * x * b - 2 * w * c) * ex + (2 * x * a + 2 * z * c) * ey +
            (2 * w * a + 2 * z * b - 4 * y * c) * ez;
    s[3] += (-4 * z * a + 2 * w * b + 2 * x * c) * ex + (-2 * w * a - 4 * z * b + 2 * y * c) * ey +
            (2 * x * a + 2 * y * b) * ez;
}
int Attitude_Update(AttitudeFilter *f, const float gyro[3], const float accel[3], const float *mag,
                    float dt, int feedback)
{
    float q[4], dq[4], omega[4], s[4] = {0}, a[3], m[3], r[3][3], earth[3], ref[3], n;
    const float gravity[3] = {0, 0, 1};
    unsigned i;
    int mode = 1;
    if (!f || !finite_vec(gyro, 3) || !isfinite(dt) || dt <= 0 || dt > 0.020001f ||
        !isfinite(f->beta) || f->beta < 0)
        return 0;
    memcpy(q, f->q, sizeof(q));
    if (!Attitude_Normalize(q))
        return 0;
    omega[0] = 0;
    for (i = 0; i < 3; ++i)
        omega[i + 1] = gyro[i];
    multiply(q, omega, dq);
    for (i = 0; i < 4; ++i)
        dq[i] *= 0.5f;
    if (feedback && f->beta > 0 && accel && unit3(accel, a))
    {
        gradient(q, gravity, a, s);
        mode = 2;
        if (mag && unit3(mag, m))
        {
            rotation(q, r);
            Attitude_Rotate(r, m, earth);
            ref[0] = sqrtf(earth[0] * earth[0] + earth[1] * earth[1]);
            ref[1] = 0;
            ref[2] = earth[2];
            if (ref[0] > 1.0e-3f)
            {
                gradient(q, ref, m, s);
                mode = 3;
            }
        }
        n = sqrtf(s[0] * s[0] + s[1] * s[1] + s[2] * s[2] + s[3] * s[3]);
        if (isfinite(n) && n > 1.0e-7f)
            for (i = 0; i < 4; ++i)
                dq[i] -= f->beta * s[i] / n;
    }
    for (i = 0; i < 4; ++i)
        q[i] += dq[i] * dt;
    if (!Attitude_Normalize(q))
        return 0;
    memcpy(f->q, q, sizeof(q));
    return mode;
}
