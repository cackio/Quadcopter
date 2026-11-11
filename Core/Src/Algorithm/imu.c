/**
 * @file    imu.c
 * @brief   传感器校准与姿态解算模块（严格按笔记 C2、C4~C7 实现）
 *
 * 笔记对应关系：
 *   C2 四元数与欧拉角   -> IMU_Quaternion_To_Euler / IMU_Euler_To_Quaternion
 *   C4 加速度计校准     -> IMU_Accel_Calib_Fit
 *   C5 陀螺仪与地磁     -> IMU_Gyro_Calib_Fit / IMU_Mag_Calib_Fit
 *   C6 梯度下降         -> IMU_Madgwick_Gradient 的重力部分
 *   C7 综合姿态解算     -> IMU_Attitude_Update（6×1 残差 + 4×6 雅可比）
 *
 * 坐标系与符号约定（与笔记一致，整份文件都按这一套写，切勿与学长代码混用）：
 *   q = [q0, q1, q2, q3] = [a, b, c, d]，q0 为实部
 *   R_q = C_b^e（机体系 -> 地理系），C_e^b = R_qᵀ（地理系 -> 机体系）
 *   R_q 的元素（笔记 C2）：
 *       [ 1-2(c²+d²)   2(bc-ad)     2(bd+ac)  ]
 *       [ 2(bc+ad)     1-2(b²+d²)   2(cd-ab)  ]
 *       [ 2(bd-ac)     2(cd+ab)     1-2(b²+c²)]
 *
 * 本文件反复用到的三个“预测向量”（全部由 R_q 或 R_qᵀ 得到）：
 *   1) 重力方向（机体系）：v = C_e^b · [0,0,1]ᵀ
 *        = [ 2(q1q3-q0q2), 2(q0q1+q2q3), q0²-q1²-q2²+q3² ]ᵀ
 *   2) 地磁方向（地理系）：h = C_b^e · m = R_q · m
 *        b' = [ √(hx²+hy²), 0, hz ]ᵀ   （把水平分量对齐地理系 x 轴 = 磁北）
 *   3) 地磁方向（机体系）：w = C_e^b · b' = R_qᵀ · b'
 *   融合残差 f = [ v - a ; w - m ]（6×1），梯度 ∇E = Jᵀ f（笔记 C6/C7）
 *
 * 单位约定：
 *   accel_body 单位 g（模长≈1）
 *   gyro_body  单位 rad/s
 *   mag_body   校准后的原始 LSB（量纲被软铁矩阵 A 吸收，模长≈1）
 *
 * ========================================================================== */

#include "imu.h"
#include "bsp.h"

/* ============================================================================
 *  全局变量定义
 * ========================================================================== */

Quaternion_t  quaternion       = {1.0f, 0.0f, 0.0f, 0.0f};
EulerAngle_t  euler_angle      = {0.0f, 0.0f, 0.0f};
Vec3f_t       accel_body       = {0.0f, 0.0f, 0.0f};   /* g，模长≈1 */
Vec3f_t       gyro_body        = {0.0f, 0.0f, 0.0f};   /* rad/s */
Vec3f_t       mag_body         = {0.0f, 0.0f, 0.0f};   /* 校准后磁场，模长≈1 */
CalibState_t  calib_state      = CALIB_PENDING;
AccelCalib_t  accel_calib      = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
GyroCalib_t   gyro_calib       = {0.0f, 0.0f, 0.0f};
MagCalib_t    mag_calib        = {0.0f, 0.0f, 0.0f,
                                  {{1.0f, 0.0f, 0.0f},
                                   {0.0f, 1.0f, 0.0f},
                                   {0.0f, 0.0f, 1.0f}}};
uint8_t       mag_calib_valid  = 0;
float         imu_dt           = IMU_DT_DEFAULT;

/* ============================================================================
 *  校准内部状态
 * ========================================================================== */

/* 采样规模说明：
 *   - 每组 200 点：加计/地磁椭球要解 6/9 个参数，姿态覆盖不足会让正规方程奇异；
 *   - 陀螺 200 点 × 10 ms ≈ 2 s 静止平均，随机噪声按 1/√n 下降；
 *   - 翻转阶段每 5 次调用（≈50 ms）采一点，200 点 ≈ 10 s，
 *     降采样是为了避免转太快时把向心加速度混进重力测量。
 */
#define CALIB_SAMPLE_SIZE       200u    /* 每组采样点数                  */
#define CALIB_ROTATE_INTERVAL   5u      /* 翻转阶段每 5 次调用采一点(≈50ms) */
#define CALIB_GYRO_MAX_RANGE    400.0f  /* 零偏采样时的静止判定(LSB，≈24°/s) */

static Vec3f_t  calib_accel_buf[CALIB_SAMPLE_SIZE];   /* 已归一化到 g */
static Vec3f_t  calib_gyro_buf[CALIB_SAMPLE_SIZE];    /* 原始 LSB     */
static Vec3f_t  calib_mag_buf[CALIB_SAMPLE_SIZE];     /* 原始 LSB     */
static uint16_t calib_sample_cnt   = 0;
static uint16_t calib_interval_cnt = 0;

typedef enum
{
    CALIB_STEP_GYRO = 0,    /* 静止采陀螺零偏 */
    CALIB_STEP_ROTATE,      /* 翻转板子采加计 + 地磁 */
    CALIB_STEP_INIT,        /* 四元数初始化 */
    CALIB_STEP_FINISH
} CalibStep_t;

static CalibStep_t calib_step      = CALIB_STEP_GYRO;
static uint32_t    imu_last_cycle  = 0;

/* ============================================================================
 *  内部工具
 * ========================================================================== */

static int IMU_Finite(float x)
{
    return (x == x) && (fabsf(x) <= 1.0e30f);   /* NaN 与 Inf 均为假 */
}

static float IMU_Clamp(float x, float lo, float hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

/* ----------------------------------------------------------------------------
 * 带主元的高斯消元：解 A·x = b（n ≤ 9）
 *
 * 用途：C4 的加计椭球拟合、C5 的地磁椭球拟合最终都归结为最小二乘的正规方程
 *     (JᵀJ) Θ = JᵀY
 * 统一在这里求解。
 *
 * 为什么用列主元：正规方程的条件数约为原矩阵条件数的平方，浮点下误差会被放大；
 * 每列选绝对值最大的行做枢轴，可显著抑制舍入误差。
 *
 * 奇异判据：eps = max|a_ij| * 1e-9，低于该值即认为矩阵奇异
 * （通常意味着采样姿态覆盖不足，例如只绕一个轴转过）。
 *
 * 注意：A 会被就地修改吗？不会，函数内部备份到 mat[9][10]。
 * ---------------------------------------------------------------------------- */
static uint8_t IMU_SolveLinear(float A[9][9], const float b[9],
                               uint8_t n, float x[9])
{
    /* 用 static：本函数只在 SensorTask 里、校准结束时调用，不存在重入问题。
     * 放到静态区可以省掉 360 字节的栈开销，避免小栈任务在拟合时栈溢出。 */
    static float mat[9][10];
    float max_a = 0.0f;
    float eps;
    uint8_t i, j, col;

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            mat[i][j] = A[i][j];
            if (fabsf(mat[i][j]) > max_a) max_a = fabsf(mat[i][j]);
        }
        mat[i][n] = b[i];
    }
    if (max_a < 1.0e-12f) return 0;

    eps = max_a * 1.0e-9f;

    for (col = 0; col < n; col++) {
        uint8_t pivot = col;
        for (i = (uint8_t)(col + 1u); i < n; i++) {
            if (fabsf(mat[i][col]) > fabsf(mat[pivot][col])) pivot = i;
        }
        if (fabsf(mat[pivot][col]) < eps) return 0;

        if (pivot != col) {
            for (j = col; j <= n; j++) {
                float tmp = mat[col][j];
                mat[col][j] = mat[pivot][j];
                mat[pivot][j] = tmp;
            }
        }

        for (i = (uint8_t)(col + 1u); i < n; i++) {
            float factor = mat[i][col] / mat[col][col];
            for (j = col; j <= n; j++) {
                mat[i][j] -= factor * mat[col][j];
            }
        }
    }

    for (i = n; i > 0u; i--) {
        uint8_t row = (uint8_t)(i - 1u);
        float sum = mat[row][n];
        for (j = (uint8_t)(row + 1u); j < n; j++) {
            sum -= mat[row][j] * x[j];
        }
        x[row] = sum / mat[row][row];
        if (!IMU_Finite(x[row])) return 0;
    }
    return 1;
}

/* ============================================================================
 *  工具函数
 * ========================================================================== */

float IMU_InvSqrt(float x)
{
    /* 可以用快速平方根倒数近似，不知道用不用得上
     * 精度足够用于归一化 */
    // float halfx = 0.5f * x;
    // float y = x;
    // long i = *(long*)&y;
    // i = 0x5f3759df - (i >> 1);
    // y = *(float*)&i;
    // y = y * (1.5f - (halfx * y * y));
    // return y;
    if (!(x > 0.0f)) return 0.0f;
    return 1.0f / sqrtf(x);
}

float IMU_Normalize(Vec3f_t *v)
{
    float n2 = v->x * v->x + v->y * v->y + v->z * v->z;
    float n;

    if (!(n2 > 1.0e-12f)) return 0.0f;
    n = sqrtf(n2);
    if (!IMU_Finite(n) || n < 1.0e-6f) return 0.0f;

    v->x /= n;
    v->y /= n;
    v->z /= n;
    return n;
}

float IMU_WrapAngle(float angle)
{
    while (angle > 3.14159265f) angle -= 2.0f * 3.14159265f;
    while (angle < -3.14159265f) angle += 2.0f * 3.14159265f;
    return angle;
}

/* ============================================================================
 *  校准：初始化 / 状态机
 * ========================================================================== */

void IMU_Calib_Init(void)
{
    // 这里初始化一些校准参数
    calib_state        = CALIB_PENDING;
    calib_step         = CALIB_STEP_GYRO;
    calib_sample_cnt   = 0;
    calib_interval_cnt = 0;

    accel_calib.offset_x = 0.0f;
    accel_calib.offset_y = 0.0f;
    accel_calib.offset_z = 0.0f;
    accel_calib.scale_x  = 1.0f;
    accel_calib.scale_y  = 1.0f;
    accel_calib.scale_z  = 1.0f;

    gyro_calib.offset_x = 0.0f;
    gyro_calib.offset_y = 0.0f;
    gyro_calib.offset_z = 0.0f;

    mag_calib.offset_x = 0.0f;
    mag_calib.offset_y = 0.0f;
    mag_calib.offset_z = 0.0f;
    mag_calib.soft_iron[0][0] = 1.0f; mag_calib.soft_iron[0][1] = 0.0f; mag_calib.soft_iron[0][2] = 0.0f;
    mag_calib.soft_iron[1][0] = 0.0f; mag_calib.soft_iron[1][1] = 1.0f; mag_calib.soft_iron[1][2] = 0.0f;
    mag_calib.soft_iron[2][0] = 0.0f; mag_calib.soft_iron[2][1] = 0.0f; mag_calib.soft_iron[2][2] = 1.0f;
    mag_calib_valid = 0;

    quaternion.w = 1.0f;
    quaternion.x = 0.0f;
    quaternion.y = 0.0f;
    quaternion.z = 0.0f;
    euler_angle.roll  = 0.0f;
    euler_angle.pitch = 0.0f;
    euler_angle.yaw   = 0.0f;

    imu_dt         = IMU_DT_DEFAULT;
    imu_last_cycle = 0;
}

void IMU_Calib_Update(void)
{
    uint16_t i;

    if (calib_state == CALIB_DONE) return;
    calib_state = CALIB_IN_PROGRESS;

    /* 校准流程共三段（每段都有 UART 提示，见 main.c 的 UartTask）：
     *   阶段 0：保持静止约 2 s   —— 200 点 × 10 ms，采陀螺零偏
     *   阶段 1：缓慢翻转约 10 s  —— 200 点 × 50 ms，采加计/地磁椭球样本
     *   阶段 2：用最后一帧原始数据求初始 roll/pitch/yaw，然后置 CALIB_DONE
     * 注意：本函数与 IMU_Attitude_Update 互斥调用，校准期间不跑融合。
     */
    switch (calib_step)
    {
    /* ---------------- 步骤 1：静止采陀螺零偏 ---------------- */
    case CALIB_STEP_GYRO:
        if (calib_sample_cnt < CALIB_SAMPLE_SIZE) {
            calib_gyro_buf[calib_sample_cnt].x = (float)Gyro_X_RAW;
            calib_gyro_buf[calib_sample_cnt].y = (float)Gyro_Y_RAW;
            calib_gyro_buf[calib_sample_cnt].z = (float)Gyro_Z_RAW;
            calib_sample_cnt++;
        }

        if (calib_sample_cnt >= CALIB_SAMPLE_SIZE) {
            /* 静止性检查：任一角速度轴峰峰值过大则重新采样 */
            float min_x = calib_gyro_buf[0].x, max_x = calib_gyro_buf[0].x;
            float min_y = calib_gyro_buf[0].y, max_y = calib_gyro_buf[0].y;
            float min_z = calib_gyro_buf[0].z, max_z = calib_gyro_buf[0].z;
            uint8_t still = 1;

            for (i = 1; i < CALIB_SAMPLE_SIZE; i++) {
                if (calib_gyro_buf[i].x < min_x) min_x = calib_gyro_buf[i].x;
                if (calib_gyro_buf[i].x > max_x) max_x = calib_gyro_buf[i].x;
                if (calib_gyro_buf[i].y < min_y) min_y = calib_gyro_buf[i].y;
                if (calib_gyro_buf[i].y > max_y) max_y = calib_gyro_buf[i].y;
                if (calib_gyro_buf[i].z < min_z) min_z = calib_gyro_buf[i].z;
                if (calib_gyro_buf[i].z > max_z) max_z = calib_gyro_buf[i].z;
            }
            if ((max_x - min_x) > CALIB_GYRO_MAX_RANGE ||
                (max_y - min_y) > CALIB_GYRO_MAX_RANGE ||
                (max_z - min_z) > CALIB_GYRO_MAX_RANGE) {
                still = 0;
            }

            if (still) {
                IMU_Gyro_Calib_Fit(calib_gyro_buf, CALIB_SAMPLE_SIZE, &gyro_calib);
                calib_step = CALIB_STEP_ROTATE;
                calib_sample_cnt = 0;
                calib_interval_cnt = 0;
            } else {
                calib_sample_cnt = 0;   /* 有晃动，重新采 */
            }
        }
        break;

    /* ---------------- 步骤 2：翻转板子采加计 + 地磁 ----------------
     * 这里只是“收集样本”，真正的拟合在收满 200 点后一次性完成：
     *   - 加计样本先除以 8192 变成 g（C4 的椭球方程以 g 为单位）；
     *   - 地磁保持原始 LSB（C5 的椭球拟合对绝对量纲不敏感，
     *     但内部会做中心化/尺度归一化）。
     * 每 5 次调用取 1 点：SensorTask 是 10 ms 周期，所以约 50 ms/点。
     */
    case CALIB_STEP_ROTATE:
        calib_interval_cnt++;
        if (calib_interval_cnt < CALIB_ROTATE_INTERVAL) break;
        calib_interval_cnt = 0;

        /* 加速度计先归一化到 g，再交给椭球拟合 */
        calib_accel_buf[calib_sample_cnt].x = (float)Accel_X_RAW / ACCEL_LSB_PER_G;
        calib_accel_buf[calib_sample_cnt].y = (float)Accel_Y_RAW / ACCEL_LSB_PER_G;
        calib_accel_buf[calib_sample_cnt].z = (float)Accel_Z_RAW / ACCEL_LSB_PER_G;

        calib_mag_buf[calib_sample_cnt].x = (float)Mag_X_RAW;
        calib_mag_buf[calib_sample_cnt].y = (float)Mag_Y_RAW;
        calib_mag_buf[calib_sample_cnt].z = (float)Mag_Z_RAW;

        calib_sample_cnt++;
        if (calib_sample_cnt >= CALIB_SAMPLE_SIZE) {
            IMU_Accel_Calib_Fit(calib_accel_buf, CALIB_SAMPLE_SIZE, &accel_calib);
            IMU_Mag_Calib_Fit(calib_mag_buf, CALIB_SAMPLE_SIZE, &mag_calib);
            calib_step = CALIB_STEP_INIT;
            calib_sample_cnt = 0;
        }
        break;

    /* ---------------- 步骤 3：四元数初始化 ---------------- */
    case CALIB_STEP_INIT:
        IMU_Quaternion_Init();
        calib_step  = CALIB_STEP_FINISH;
        calib_state = CALIB_DONE;
        break;

    default:
        calib_state = CALIB_DONE;
        break;
    }
}

uint8_t IMU_Calib_Done(void)
{
    return (calib_state == CALIB_DONE) ? 1u : 0u;
}

uint8_t IMU_Calib_Stage(void)
{
    if (calib_state == CALIB_DONE) return 2u;
    if (calib_step == CALIB_STEP_GYRO) return 0u;
    return 1u;
}

/* ============================================================================
 *  校准拟合
 * ========================================================================== */

void IMU_Accel_Calib_Fit(const Vec3f_t *samples, uint16_t count, AccelCalib_t *calib)
{
    /* ------------------------------------------------------------------------
     * 笔记 C4 第 2 节：加计椭球的 6 参数线性化
     *
     * 原始椭球：Sx²(X-Ox)² + Sy²(Y-Oy)² + Sz²(Z-Oz)² = 1
     * 展开后整体除以常数项，得到
     *     θ1X² + θ2Y² + θ3Z² + θ4X + θ5Y + θ6Z = 1
     * 其中
     *     θ1 = Sx²/K,  θ2 = Sy²/K,  θ3 = Sz²/K
     *     θ4 = -2Sx²Ox/K, θ5 = -2Sy²Oy/K, θ6 = -2Sz²Oz/K
     *     K  = Sx²Ox² + Sy²Oy² + Sz²Oz² - 1
     *
     * 还原（C4 第 2.3/2.4 节）：
     *     Ox = -θ4/(2θ1), Oy = -θ5/(2θ2), Oz = -θ6/(2θ3)
     *     Sx = √(θ1D),    Sy = √(θ2D),    Sz = √(θ3D)
     *     D  = 1/(1+S)
     *     S  = θ4²/(4θ1) + θ5²/(4θ2) + θ6²/(4θ3)
     *
     * 最小二乘：每条样本给出方程 [X²,Y²,Z²,X,Y,Z]·Θ = 1，
     *     Θ = (AᵀA)⁻¹ AᵀY（Y 全为 1）
     *
     * 采样已在 CALIB_STEP_ROTATE 中除以 ACCEL_LSB_PER_G，
     * 因此这里 X/Y/Z 的单位是 g，理想结果应是 Ox≈0、Sx≈1。
     * ------------------------------------------------------------------------ */
    /* 同上：static 省栈（ATA 一个就 324 字节，SensorTask 的栈撑不住） */
    static float ATA[9][9];
    static float ATY[9];
    static float theta[9];
    float trace, ridge;
    float s, d;
    uint16_t i;
    uint8_t j, k;
    uint8_t ok;

    for (j = 0; j < 9u; j++) {
        ATY[j] = 0.0f;
        for (k = 0; k < 9u; k++) ATA[j][k] = 0.0f;
    }

    for (i = 0; i < count; i++) {
        float x = samples[i].x;
        float y = samples[i].y;
        float z = samples[i].z;
        float row[6];
        row[0] = x * x; row[1] = y * y; row[2] = z * z;
        row[3] = x;     row[4] = y;     row[5] = z;

        for (j = 0; j < 6u; j++) {
            for (k = 0; k < 6u; k++) ATA[j][k] += row[j] * row[k];
            ATY[j] += row[j];      /* Y 全为 1 */
        }
    }

    /* 轻微岭正则，避免采样姿态不足时矩阵奇异 */
    trace = ATA[0][0] + ATA[1][1] + ATA[2][2];
    ridge = trace * 1.0e-6f + 1.0e-9f;
    for (j = 0; j < 6u; j++) ATA[j][j] += ridge;

    ok = IMU_SolveLinear(ATA, ATY, 6u, theta);

    if (!ok || theta[0] <= 0.0f || theta[1] <= 0.0f || theta[2] <= 0.0f) {
        calib->offset_x = 0.0f; calib->offset_y = 0.0f; calib->offset_z = 0.0f;
        calib->scale_x  = 1.0f; calib->scale_y  = 1.0f; calib->scale_z  = 1.0f;
        return;
    }

    s = theta[3] * theta[3] / (4.0f * theta[0])
      + theta[4] * theta[4] / (4.0f * theta[1])
      + theta[5] * theta[5] / (4.0f * theta[2]);
    d = 1.0f / (1.0f + s);

    if (!IMU_Finite(d) || d <= 0.0f) {
        calib->offset_x = 0.0f; calib->offset_y = 0.0f; calib->offset_z = 0.0f;
        calib->scale_x  = 1.0f; calib->scale_y  = 1.0f; calib->scale_z  = 1.0f;
        return;
    }

    calib->offset_x = -theta[3] / (2.0f * theta[0]);
    calib->offset_y = -theta[4] / (2.0f * theta[1]);
    calib->offset_z = -theta[5] / (2.0f * theta[2]);
    calib->scale_x  = sqrtf(theta[0] * d);
    calib->scale_y  = sqrtf(theta[1] * d);
    calib->scale_z  = sqrtf(theta[2] * d);

    /* 合理性检查：不合理的拟合结果直接退回默认值，避免 NaN 传播 */
    if (!IMU_Finite(calib->offset_x) || !IMU_Finite(calib->offset_y) ||
        !IMU_Finite(calib->offset_z) || !IMU_Finite(calib->scale_x)  ||
        !IMU_Finite(calib->scale_y)  || !IMU_Finite(calib->scale_z)  ||
        fabsf(calib->offset_x) > 3.0f || fabsf(calib->offset_y) > 3.0f ||
        fabsf(calib->offset_z) > 3.0f ||
        calib->scale_x < 0.2f || calib->scale_x > 5.0f ||
        calib->scale_y < 0.2f || calib->scale_y > 5.0f ||
        calib->scale_z < 0.2f || calib->scale_z > 5.0f)
    {
        calib->offset_x = 0.0f; calib->offset_y = 0.0f; calib->offset_z = 0.0f;
        calib->scale_x  = 1.0f; calib->scale_y  = 1.0f; calib->scale_z  = 1.0f;
    }
}

void IMU_Gyro_Calib_Fit(const Vec3f_t *samples, uint16_t count, GyroCalib_t *calib)
{
    /* ------------------------------------------------------------------------
     * 笔记 C5 第 1 节：静止时角速度真值为 0，因此零偏就是读数的均值
     *     bias_raw = (1/n) Σ gyro_raw
     * 输出统一换算到 rad/s（这样姿态更新里可以直接做减法）：
     *     bias_rad = bias_raw · DEG_TO_RAD / GYRO_LSB_PER_DPS
     * 其中 16.4 LSB/(°/s) 对应 GYRO_CONFIG = ±2000dps（bsp.c 里已写入 0x18）。
     * ------------------------------------------------------------------------ */
    float sx = 0.0f, sy = 0.0f, sz = 0.0f;
    uint16_t i;

    for (i = 0; i < count; i++) {
        sx += samples[i].x;
        sy += samples[i].y;
        sz += samples[i].z;
    }

    calib->offset_x = (sx / (float)count) * DEG_TO_RAD / GYRO_LSB_PER_DPS;
    calib->offset_y = (sy / (float)count) * DEG_TO_RAD / GYRO_LSB_PER_DPS;
    calib->offset_z = (sz / (float)count) * DEG_TO_RAD / GYRO_LSB_PER_DPS;

    if (!IMU_Finite(calib->offset_x) || !IMU_Finite(calib->offset_y) ||
        !IMU_Finite(calib->offset_z)) {
        calib->offset_x = 0.0f; calib->offset_y = 0.0f; calib->offset_z = 0.0f;
    }
}

void IMU_Mag_Calib_Fit(const Vec3f_t *samples, uint16_t count, MagCalib_t *calib)
{
    /* 笔记 C5：
     *   A_xx x² + A_yy y² + A_zz z² + B_xy xy + B_xz xz + B_yz yz
     *   + C_x x + C_y y + C_z z = 1
     *   M = [A_xx, B_xy/2, B_xz/2; ...]，C = [C_x, C_y, C_z]
     *   b = -½M⁻¹C，M = AᵀA，A = Lᵀ（Cholesky）
     *
     * 数值处理：HMC 原始 LSB 硬铁偏置可达数百，而磁场变化半径也在数百，
     * 直接对原始量构建正规方程条件数极差（x²、x、1 三列近似线性相关），
     * 岭正则会把解压向 0。这里先减去样本均值、再按平均半径归一化，
     * 拟合完成后把 b 与 A 变换回原始量纲（数学上完全等价）。
     */
    /* 同上：static 省栈（JTJ 324 字节 + mat3 36 字节） */
    static float JTJ[9][9];
    static float JTr[9];
    static float theta[9];
    static float mat3[3][3];
    float M[3][3];
    float C[3];
    float Minv[3][3];
    float det;
    float trace, ridge;
    float mean[3] = {0.0f, 0.0f, 0.0f};
    float rscale = 0.0f;
    float ms, L[3][3];
    float bx, by, bz;
    uint16_t i;
    uint8_t j, k;
    uint8_t ok, chol_ok;

    /* 默认：零硬铁 + 单位软铁 */
    calib->offset_x = 0.0f; calib->offset_y = 0.0f; calib->offset_z = 0.0f;
    calib->soft_iron[0][0] = 1.0f; calib->soft_iron[0][1] = 0.0f; calib->soft_iron[0][2] = 0.0f;
    calib->soft_iron[1][0] = 0.0f; calib->soft_iron[1][1] = 1.0f; calib->soft_iron[1][2] = 0.0f;
    calib->soft_iron[2][0] = 0.0f; calib->soft_iron[2][1] = 0.0f; calib->soft_iron[2][2] = 1.0f;
    mag_calib_valid = 0;

    if (count < 9u) return;

    /* ---- 数据中心化：均值当作粗硬铁中心 ---- */
    for (i = 0; i < count; i++) {
        mean[0] += samples[i].x;
        mean[1] += samples[i].y;
        mean[2] += samples[i].z;
    }
    mean[0] /= (float)count;
    mean[1] /= (float)count;
    mean[2] /= (float)count;

    /* ---- 尺度归一化：样本到均值的平均距离 ---- */
    for (i = 0; i < count; i++) {
        float dx = samples[i].x - mean[0];
        float dy = samples[i].y - mean[1];
        float dz = samples[i].z - mean[2];
        rscale += sqrtf(dx * dx + dy * dy + dz * dz);
    }
    rscale /= (float)count;

    if (!IMU_Finite(rscale) || rscale < 1.0e-3f) return;

    for (j = 0; j < 9u; j++) {
        JTr[j] = 0.0f;
        for (k = 0; k < 9u; k++) JTJ[j][k] = 0.0f;
    }

    for (i = 0; i < count; i++) {
        float x = (samples[i].x - mean[0]) / rscale;
        float y = (samples[i].y - mean[1]) / rscale;
        float z = (samples[i].z - mean[2]) / rscale;
        float row[9];
        row[0] = x * x; row[1] = y * y; row[2] = z * z;
        row[3] = x * y; row[4] = x * z; row[5] = y * z;
        row[6] = x;     row[7] = y;     row[8] = z;

        for (j = 0; j < 9u; j++) {
            for (k = 0; k < 9u; k++) JTJ[j][k] += row[j] * row[k];
            JTr[j] += row[j];
        }
    }

    trace = JTJ[0][0] + JTJ[1][1] + JTJ[2][2];
    ridge = trace * 1.0e-6f + 1.0e-9f;
    for (j = 0; j < 9u; j++) JTJ[j][j] += ridge;

    ok = IMU_SolveLinear(JTJ, JTr, 9u, theta);
    if (!ok) {
        /* 退化为“均值 + 单位软铁”，仍能消除大部分硬铁 */
        calib->offset_x = mean[0];
        calib->offset_y = mean[1];
        calib->offset_z = mean[2];
        calib->soft_iron[0][0] = 1.0f / rscale;
        calib->soft_iron[1][1] = 1.0f / rscale;
        calib->soft_iron[2][2] = 1.0f / rscale;
        return;
    }

    M[0][0] = theta[0];
    M[1][1] = theta[1];
    M[2][2] = theta[2];
    M[0][1] = M[1][0] = theta[3] * 0.5f;
    M[0][2] = M[2][0] = theta[4] * 0.5f;
    M[1][2] = M[2][1] = theta[5] * 0.5f;

    C[0] = theta[6];
    C[1] = theta[7];
    C[2] = theta[8];

    det = M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1])
        - M[0][1] * (M[1][0] * M[2][2] - M[1][2] * M[2][0])
        + M[0][2] * (M[1][0] * M[2][1] - M[1][1] * M[2][0]);

    if (!IMU_Finite(det) || fabsf(det) < 1.0e-20f) {
        calib->offset_x = mean[0];
        calib->offset_y = mean[1];
        calib->offset_z = mean[2];
        calib->soft_iron[0][0] = 1.0f / rscale;
        calib->soft_iron[1][1] = 1.0f / rscale;
        calib->soft_iron[2][2] = 1.0f / rscale;
        return;
    }

    Minv[0][0] = (M[1][1] * M[2][2] - M[1][2] * M[2][1]) / det;
    Minv[0][1] = (M[0][2] * M[2][1] - M[0][1] * M[2][2]) / det;
    Minv[0][2] = (M[0][1] * M[1][2] - M[0][2] * M[1][1]) / det;
    Minv[1][0] = (M[1][2] * M[2][0] - M[1][0] * M[2][2]) / det;
    Minv[1][1] = (M[0][0] * M[2][2] - M[0][2] * M[2][0]) / det;
    Minv[1][2] = (M[0][2] * M[1][0] - M[0][0] * M[1][2]) / det;
    Minv[2][0] = (M[1][0] * M[2][1] - M[1][1] * M[2][0]) / det;
    Minv[2][1] = (M[0][1] * M[2][0] - M[0][0] * M[2][1]) / det;
    Minv[2][2] = (M[0][0] * M[1][1] - M[0][1] * M[1][0]) / det;

    /* ------------------------------------------------------------------------
     * 硬铁偏置还原（笔记 C5 第 108 行 b = -½M⁻¹C）
     *
     * 拟合是在归一化坐标 x_n = (m - mean)/rscale 下完成的，所以先得到
     *     b_n = -½ M_n⁻¹ C_n
     * 再换回原始 LSB 坐标：
     *     b_raw = mean + rscale · b_n
     * 这里的 mean 只是“粗中心”，b_n 才是椭球相对均值的真实偏移；
     * 两者相加才是最终硬铁偏置。
     * ------------------------------------------------------------------------ */
    bx = -0.5f * (Minv[0][0] * C[0] + Minv[0][1] * C[1] + Minv[0][2] * C[2]);
    by = -0.5f * (Minv[1][0] * C[0] + Minv[1][1] * C[1] + Minv[1][2] * C[2]);
    bz = -0.5f * (Minv[2][0] * C[0] + Minv[2][1] * C[1] + Minv[2][2] * C[2]);

    calib->offset_x = mean[0] + rscale * bx;
    calib->offset_y = mean[1] + rscale * by;
    calib->offset_z = mean[2] + rscale * bz;

    if (!IMU_Finite(calib->offset_x) || !IMU_Finite(calib->offset_y) ||
        !IMU_Finite(calib->offset_z) ||
        fabsf(calib->offset_x) > 8000.0f ||
        fabsf(calib->offset_y) > 8000.0f ||
        fabsf(calib->offset_z) > 8000.0f) {
        calib->offset_x = mean[0];
        calib->offset_y = mean[1];
        calib->offset_z = mean[2];
        calib->soft_iron[0][0] = 1.0f / rscale;
        calib->soft_iron[1][1] = 1.0f / rscale;
        calib->soft_iron[2][2] = 1.0f / rscale;
        return;
    }

    /* 归一化坐标下的 M 再 Cholesky，避免原始量纲太小导致数值问题 */
    ms = (M[0][0] + M[1][1] + M[2][2]) / 3.0f;
    if (!IMU_Finite(ms) || ms <= 0.0f) return;

    for (j = 0; j < 3u; j++)
        for (k = 0; k < 3u; k++)
            mat3[j][k] = M[j][k] / ms;

    chol_ok = 1;
    L[0][0] = sqrtf(mat3[0][0]);
    if (!(L[0][0] > 1.0e-6f)) chol_ok = 0;
    if (chol_ok) {
        L[1][0] = mat3[1][0] / L[0][0];
        L[1][1] = sqrtf(mat3[1][1] - L[1][0] * L[1][0]);
        if (!(L[1][1] > 1.0e-6f)) chol_ok = 0;
    }
    if (chol_ok) {
        L[2][0] = mat3[2][0] / L[0][0];
        L[2][1] = (mat3[2][1] - L[2][0] * L[1][0]) / L[1][1];
        L[2][2] = sqrtf(mat3[2][2] - L[2][0] * L[2][0] - L[2][1] * L[2][1]);
        if (!(L[2][2] > 1.0e-6f)) chol_ok = 0;
    }

    if (!chol_ok) {
        return;   /* 保留上面算出的硬铁偏置，软铁退回单位阵 */
    }

    {
        /* --------------------------------------------------------------------
         * 软铁矩阵还原（笔记 C5 第四节：M = AᵀA，Cholesky 取 A = Lᵀ）
         *
         * 归一化坐标下的二次型是 M_n，换回原始坐标时
         *     x_n = (m - b)/rscale
         *     (m-b)ᵀ M_raw (m-b) = x_nᵀ M_n x_n  =>  M_raw = M_n / rscale²
         * 因此
         *     A_raw = √ms / rscale · Lᵀ，其中 M_n/ms = L Lᵀ
         *
         * 严格来说，拟合方程右端固定为 1 时
         *     (m-b)ᵀ M (m-b) = 1 + bᵀMb
         * 会多出一个整体尺度因子；但姿态解算里对 m 做了归一化（只取方向），
         * 所以这个尺度不影响融合结果，A 只需保证球面形状正确。
         * -------------------------------------------------------------------- */
        float s = sqrtf(ms) / rscale;
        calib->soft_iron[0][0] = s * L[0][0]; calib->soft_iron[0][1] = s * L[1][0]; calib->soft_iron[0][2] = s * L[2][0];
        calib->soft_iron[1][0] = 0.0f;        calib->soft_iron[1][1] = s * L[1][1]; calib->soft_iron[1][2] = s * L[2][1];
        calib->soft_iron[2][0] = 0.0f;        calib->soft_iron[2][1] = 0.0f;        calib->soft_iron[2][2] = s * L[2][2];
    }

    for (j = 0; j < 3u; j++)
        for (k = 0; k < 3u; k++)
            if (!IMU_Finite(calib->soft_iron[j][k])) return;

    mag_calib_valid = 1;
}

/* ============================================================================
 *  四元数初始化
 * ========================================================================== */

void IMU_Quaternion_Init(void)
{
    /* ------------------------------------------------------------------------
     * 初始 roll / pitch（笔记 C4 + C2）
     *
     * 加计静止时测得的是重力在机体系的方向：
     *     a = C_e^b · [0,0,1]ᵀ
     *       = [ 2(q1q3-q0q2), 2(q0q1+q2q3), 1-2(q1²+q2²) ]ᵀ
     * 令 yaw≈0、先只看 roll/pitch，则
     *     a ≈ [ -sinθ, cosθ·sinφ, cosθ·cosφ ]ᵀ
     * 反解得
     *     roll  φ = atan2(ay, az)
     *     pitch θ = asin(-ax)
     * 横滚/俯仰由加计一次给准，后续 Madgwick 主要只需要修 yaw。
     * ------------------------------------------------------------------------ */
    float ax = accel_calib.scale_x * ((float)Accel_X_RAW / ACCEL_LSB_PER_G - accel_calib.offset_x);
    float ay = accel_calib.scale_y * ((float)Accel_Y_RAW / ACCEL_LSB_PER_G - accel_calib.offset_y);
    float az = accel_calib.scale_z * ((float)Accel_Z_RAW / ACCEL_LSB_PER_G - accel_calib.offset_z);
    float mx0 = (float)Mag_X_RAW - mag_calib.offset_x;
    float my0 = (float)Mag_Y_RAW - mag_calib.offset_y;
    float mz0 = (float)Mag_Z_RAW - mag_calib.offset_z;
    float mx, my, mz;
    float an;
    float roll, pitch;
    EulerAngle_t e;

    an = sqrtf(ax * ax + ay * ay + az * az);
    if (an > 0.1f && IMU_Finite(an)) {
        ax /= an; ay /= an; az /= an;
    } else {
        ax = 0.0f; ay = 0.0f; az = 1.0f;
    }

    roll  = atan2f(ay, az);
    pitch = asinf(IMU_Clamp(-ax, -1.0f, 1.0f));

    mx = mag_calib.soft_iron[0][0] * mx0 + mag_calib.soft_iron[0][1] * my0 + mag_calib.soft_iron[0][2] * mz0;
    my = mag_calib.soft_iron[1][0] * mx0 + mag_calib.soft_iron[1][1] * my0 + mag_calib.soft_iron[1][2] * mz0;
    mz = mag_calib.soft_iron[2][0] * mx0 + mag_calib.soft_iron[2][1] * my0 + mag_calib.soft_iron[2][2] * mz0;

    /* ------------------------------------------------------------------------
     * 初始 yaw（对应笔记 C7 的 b' 约定）
     *
     * 融合时地磁参考被固定为 b' = [√(hx²+hy²), 0, hz]，等价于“地理系 x 轴 = 磁北”。
     * 水平放置时（roll=pitch=0），机体系下的地磁水平分量为
     *     m_h = B·[cosα, sinα]ᵀ（α 为机体系下磁北方向角）
     * 收敛不动点要求 h = R_qᵀ m 的水平分量落在 +x 上，反解即
     *     ψ = atan2(-my, mx)
     *
     * 注意：这里没有做倾斜补偿（没有先用 roll/pitch 旋转 m），
     * 所以只是近似初值；Madgwick 的梯度项会在 1~2 s 内把它收敛到正确值。
     * ------------------------------------------------------------------------ */
    e.roll  = roll;
    e.pitch = pitch;
    if (IMU_Finite(mx) && IMU_Finite(my) && (mx * mx + my * my) > 1.0e-6f) {
        e.yaw = atan2f(-my, mx);
    } else {
        e.yaw = 0.0f;
    }

    (void)mz;
    IMU_Euler_To_Quaternion(&e, &quaternion);

    /* 初始姿态角同步输出 */
    euler_angle.roll  = e.roll;
    euler_angle.pitch = e.pitch;
    euler_angle.yaw   = e.yaw;
}

/* ============================================================================
 *  姿态解算
 * ========================================================================== */

void IMU_Attitude_Update(void)
{
    uint32_t now, delta;
    float dt;
    float ax, ay, az;
    float wx, wy, wz;
    float mx0, my0, mz0, mx, my, mz;
    float q0, q1, q2, q3;
    float dq0, dq1, dq2, dq3;
    float g0, g1, g2, g3, gn;
    float grad[4];
    Vec3f_t a, m;
    float an, mn;
    float beta;
    float q0n, q1n, q2n, q3n, qn;

    /* ---- 步骤 0：DWT 实测采样周期（C7 的 Δt）----
     *   dt = ΔCYCCNT / SystemCoreClock
     *   CYCCNT 是 32 位自由计数器，now - last 用无符号减法，天然处理回绕。
     *   保护：过小(<0.5 ms)认为无效，过大(>50 ms)截断，
     *   避免某次任务被阻塞（例如气压计阻塞 20 ms）时积分爆掉。
     */
    now   = DWT->CYCCNT;
    delta = now - imu_last_cycle;          /* 无符号相减，天然处理 32 位回绕 */
    imu_last_cycle = now;
    dt = (float)delta / (float)SystemCoreClock;
    if (!(dt > 0.0005f)) dt = IMU_DT_DEFAULT;
    if (dt > 0.05f) dt = 0.05f;
    imu_dt = dt;

    /* ---- 步骤 1：应用校准参数（C4/C5 校准的逆变换）----
     *   加计：a = Sx·(raw / 8192 - Ox)，单位 g
     *   陀螺：ω = raw·DEG_TO_RAD / 16.4 - bias_rad
     *   地磁：m = A·(raw - b)
     *   注意软铁：必须先算好 mx0/my0/mz0 再乘 A，绝不能原地覆盖，
     *   否则第二行用到的 my0 会被第一行的结果污染。
     */
    ax = accel_calib.scale_x * ((float)Accel_X_RAW / ACCEL_LSB_PER_G - accel_calib.offset_x);
    ay = accel_calib.scale_y * ((float)Accel_Y_RAW / ACCEL_LSB_PER_G - accel_calib.offset_y);
    az = accel_calib.scale_z * ((float)Accel_Z_RAW / ACCEL_LSB_PER_G - accel_calib.offset_z);

    wx = (float)Gyro_X_RAW * DEG_TO_RAD / GYRO_LSB_PER_DPS - gyro_calib.offset_x;
    wy = (float)Gyro_Y_RAW * DEG_TO_RAD / GYRO_LSB_PER_DPS - gyro_calib.offset_y;
    wz = (float)Gyro_Z_RAW * DEG_TO_RAD / GYRO_LSB_PER_DPS - gyro_calib.offset_z;

    mx0 = (float)Mag_X_RAW - mag_calib.offset_x;
    my0 = (float)Mag_Y_RAW - mag_calib.offset_y;
    mz0 = (float)Mag_Z_RAW - mag_calib.offset_z;
    mx = mag_calib.soft_iron[0][0] * mx0 + mag_calib.soft_iron[0][1] * my0 + mag_calib.soft_iron[0][2] * mz0;
    my = mag_calib.soft_iron[1][0] * mx0 + mag_calib.soft_iron[1][1] * my0 + mag_calib.soft_iron[1][2] * mz0;
    mz = mag_calib.soft_iron[2][0] * mx0 + mag_calib.soft_iron[2][1] * my0 + mag_calib.soft_iron[2][2] * mz0;

    accel_body.x = ax; accel_body.y = ay; accel_body.z = az;
    gyro_body.x  = wx; gyro_body.y  = wy; gyro_body.z  = wz;
    mag_body.x   = mx; mag_body.y   = my; mag_body.z   = mz;

    /* ---- 步骤 2：只保留方向 ----
     *   融合残差比较的是单位方向，所以 a、m 都要归一化。
     *   IMU_Normalize 返回原模长：加计约 1 g，地磁约 1。
     *   若某帧读数无效（模长≈0），返回 0，后面会跳过对应的修正项。
     */
    a.x = ax; a.y = ay; a.z = az;
    m.x = mx; m.y = my; m.z = mz;
    an = IMU_Normalize(&a);
    mn = IMU_Normalize(&m);

    /* ---- 步骤 3：陀螺角速度驱动四元数（四元数微分方程）----
     *     q̇_ω = ½ · q ⊗ [0, ωx, ωy, ωz]
     * 设 q=[q0,q1,q2,q3]、ω=[wx,wy,wz]，展开四个分量：
     *     q̇0 = ½( -q1wx - q2wy - q3wz )
     *     q̇1 = ½(  q0wx + q2wz - q3wy )
     *     q̇2 = ½(  q0wy - q1wz + q3wx )
     *     q̇3 = ½(  q0wz + q1wy - q2wx )
     * 这一步只是“预测”（纯陀螺积分，会漂移）；误差留给下一步梯度修正。
     */
    q0 = quaternion.w; q1 = quaternion.x; q2 = quaternion.y; q3 = quaternion.z;

    dq0 = 0.5f * (-q1 * wx - q2 * wy - q3 * wz);
    dq1 = 0.5f * ( q0 * wx + q2 * wz - q3 * wy);
    dq2 = 0.5f * ( q0 * wy - q1 * wz + q3 * wx);
    dq3 = 0.5f * ( q0 * wz + q1 * wy - q2 * wx);

    /* ---- 步骤 4：∇E = Jᵀf（加计 3 + 地磁 3 = 6×1 残差，笔记 C6/C7）----
     *   f = [ v - a ; w - m ]，目标函数 E = ½ fᵀf，梯度 ∇E = Jᵀf。
     *   雅可比展开全部写在 IMU_Madgwick_Gradient 函数头注释里。
     *   加计无效（an ≤ 0.1 g，说明在剧烈机动或掉落）时不做修正，
     *   地磁无效（mn ≤ 0.01）时自动退化为只用重力的 3×1 残差。
     */
    g0 = 0.0f; g1 = 0.0f; g2 = 0.0f; g3 = 0.0f;
    if (an > 0.1f) {
        IMU_Madgwick_Gradient(&quaternion, &a, &m,
                              (mn > 0.01f) ? 1u : 0u, grad);
        g0 = grad[0];
        g1 = grad[1];
        g2 = grad[2];
        g3 = grad[3];
    }

    /* ---- 步骤 5：融合更新（笔记 C7 第 175 行）----
     *     q_t = q_{t-1} + ( q̇_ω − β·∇E/‖∇E‖ ) · Δt
     * 先归一化 ∇E，保证不同姿态下修正步长量级一致；
     * 陀螺项负责高频响应，梯度项负责把长期漂移拉回重力方向与磁北。
     * β = IMU_BETA_DEFAULT = 0.1，对应 100 Hz 采样。
     */
    beta = IMU_BETA_DEFAULT;
    gn = sqrtf(g0 * g0 + g1 * g1 + g2 * g2 + g3 * g3);
    if (IMU_Finite(gn) && gn > 1.0e-6f) {
        g0 /= gn; g1 /= gn; g2 /= gn; g3 /= gn;
    } else {
        g0 = 0.0f; g1 = 0.0f; g2 = 0.0f; g3 = 0.0f;
    }

    q0n = q0 + (dq0 - beta * g0) * dt;
    q1n = q1 + (dq1 - beta * g1) * dt;
    q2n = q2 + (dq2 - beta * g2) * dt;
    q3n = q3 + (dq3 - beta * g3) * dt;

    /* ---- 步骤 6：强制归一化（C7 明确要求）----
     *   一阶欧拉积分会让模长缓慢偏离 1；
     *   不归一化的话 R_q 就不再是纯旋转矩阵，重力/地磁预测都会失真。
     *   顺带处理异常：若出现 NaN 或模长≈0，则丢弃本帧结果，保持上一帧姿态。
     */
    qn = sqrtf(q0n * q0n + q1n * q1n + q2n * q2n + q3n * q3n);
    if (IMU_Finite(qn) && qn > 1.0e-6f) {
        quaternion.w = q0n / qn;
        quaternion.x = q1n / qn;
        quaternion.y = q2n / qn;
        quaternion.z = q3n / qn;
    }

    /* ---- 步骤 7：四元数 -> 欧拉角（公式见 IMU_Quaternion_To_Euler）---- */
    IMU_Quaternion_To_Euler(&quaternion, &euler_angle);
}

void IMU_Quaternion_To_Euler(const Quaternion_t *q, EulerAngle_t *e)
{
    /* ------------------------------------------------------------------------
     * 笔记 C2 第二节。先把 a..d 与 R_q 的对应写清楚，避免轴序混淆：
     *
     *   R_q = C_b^e =
     *     [ 1-2(c²+d²)   2(bc-ad)     2(bd+ac) ]
     *     [ 2(bc+ad)     1-2(b²+d²)   2(cd-ab) ]
     *     [ 2(bd-ac)     2(cd+ab)     1-2(b²+c²)]
     *
     * Z-Y-X 提取式必须直接从上面这个 R_q 的对应元素取，不能凭记忆写：
     *   R[2][0] = -sinθ   -> pitch θ = asin(-R[2][0]) = asin(2bd - 2ac)
     *   R[2][1] = sinφcosθ-> roll  φ = atan2(R[2][1], R[2][2])
     *                                     = atan2(2cd + 2ab, 1 - 2b² - 2c²)
     *   R[1][0] = cosθsinψ-> yaw   ψ = atan2(R[1][0], R[0][0])
     *                                     = atan2(2bc + 2ad, 1 - 2c² - 2d²)
     *
     * 三个交叉项全部是“加”：
     *   pitch = asin(2(q0q2 - q1q3))
     *   roll  = atan2(2(q0q1 + q2q3), 1 - 2(q1² + q2²))
     *   yaw   = atan2(2(q0q3 + q1q2), 1 - 2(q2² + q3²))
     *
     * 写成“减”等价于把 q 取共轭，也就是把 C_b^e 当成 C_e^b 用：
     *   单轴旋转时看不太出来（交叉项为 0，两种写法结果一样），
     *   一旦 roll/pitch/yaw 同时非零就会串轴 ——
     *   偏航 155° 时滚转 1° 会算出俯仰 -20° 这种完全对不上的假值，
     *   3D 模型表现为“板子往左滚、模型往右滚”，怎么转都跟不到位。
     *
     * 其中 a=q0(w), b=q1(x), c=q2(y), d=q3(z)。
     * asin 的定义域是 [-1,1]，浮点误差可能给出 1.0000001，
     * 必须先 clamp，否则返回 NaN。
     * ------------------------------------------------------------------------ */
    float q0 = q->w, q1 = q->x, q2 = q->y, q3 = q->z;
    float sin_pitch = 2.0f * (q0 * q2 - q1 * q3);

    sin_pitch = IMU_Clamp(sin_pitch, -1.0f, 1.0f);

    e->pitch = asinf(sin_pitch);
    e->yaw   = atan2f(2.0f * (q0 * q3 + q1 * q2),
                      1.0f - 2.0f * (q2 * q2 + q3 * q3));
    e->roll  = atan2f(2.0f * (q0 * q1 + q2 * q3),
                      1.0f - 2.0f * (q1 * q1 + q2 * q2));
}

void IMU_Euler_To_Quaternion(const EulerAngle_t *e, Quaternion_t *q)
{
    /* ------------------------------------------------------------------------
     * 笔记 C2：q_net = q_z ⊗ q_y ⊗ q_x（Z-Y-X 外旋），展开就是
     *   a = cφcθcψ + sφsθsψ
     *   b = sφcθcψ - cφsθsψ
     *   c = cφsθcψ + sφcθsψ
     *   d = cφcθsψ - sφsθcψ
     * 半角：φ=roll/2, θ=pitch/2, ψ=yaw/2
     * 必须和 IMU_Quaternion_To_Euler 用同一套约定（C_b^e），
     * 否则这里转出来的四元数跟加计算出来的 roll/pitch 对不上，
     * 上电初始对准那一下就会被梯度项往回拽，姿态先歪一段时间再慢慢回正。
     * 本式与 IMU_Quaternion_To_Euler 严格互逆（已做数值自检：
     * 任取一组欧拉角 -> 四元数 -> 欧拉角，误差 < 0.1°）。
     * 用途：校准时由初始 roll/pitch/yaw 构造四元数。
     * ------------------------------------------------------------------------ */
    float phi   = e->roll  * 0.5f;
    float theta = e->pitch * 0.5f;
    float psi   = e->yaw   * 0.5f;

    float cphi = cosf(phi),   sphi = sinf(phi);
    float ctheta = cosf(theta), stheta = sinf(theta);
    float cpsi = cosf(psi),   spsi = sinf(psi);

    q->w = cphi * ctheta * cpsi + sphi * stheta * spsi;
    q->x = sphi * ctheta * cpsi - cphi * stheta * spsi;
    q->y = cphi * stheta * cpsi + sphi * ctheta * spsi;
    q->z = cphi * ctheta * spsi - sphi * stheta * cpsi;
}

/* ============================================================================
 *  Madgwick 梯度（笔记 C6/C7）
 * ========================================================================== */

void IMU_Madgwick_Gradient(const Quaternion_t *q,
                           const Vec3f_t *a,
                           const Vec3f_t *m,
                           uint8_t use_mag,
                           float grad[4])
{
    /* ========================================================================
     * ∇E = Jᵀf 的完整展开（笔记 C6 重力部分 + C7 地磁部分）
     *
     * 一、重力（C6）
     *   预测重力方向（机体系）：v = C_e^b · [0,0,1]ᵀ
     *       vx = 2(q1q3 - q0q2)
     *       vy = 2(q0q1 + q2q3)
     *       vz = q0² - q1² - q2² + q3²   （等价于 1-2q1²-2q2²）
     *   残差 f0..f2 = v - a
     *
     *   重力雅可比 Jg = ∂v/∂q（3×4），按上面 vz 的未归一化写法：
     *       [ -2q2   2q3  -2q0   2q1 ]
     *       [  2q1   2q0   2q3   2q2 ]
     *       [  2q0  -2q1  -2q2   2q3 ]
     *   转置相乘后重力梯度：
     *       e0 = -2q2·f0 + 2q1·f1 + 2q0·f2
     *       e1 =  2q3·f0 + 2q0·f1 - 2q1·f2
     *       e2 = -2q0·f0 + 2q3·f1 - 2q2·f2
     *       e3 =  2q1·f0 + 2q2·f1 + 2q3·f2
     *
     * 二、地磁（C7）
     *   1) h = R_q·m，得到地磁在地理系中的方向
     *   2) b' = [bx, 0, bz]ᵀ，bx = √(hx²+hy²)，即“地理系 x 轴 = 磁北”
     *   3) w = R_qᵀ·b'，得到预测的机体系地磁方向
     *   4) 残差 f3..f5 = w - m
     *   5) w 对 q 的偏导（4×3，每行对应一个 q 分量）：
     *      ∂w/∂q0 = [ -2bz·q2,            2bz·q1 - 2bx·q3,   2bx·q2            ]
     *      ∂w/∂q1 = [  2bz·q3,            2bx·q2 + 2bz·q0,   2bx·q3 - 4bz·q1    ]
     *      ∂w/∂q2 = [ -4bx·q2 - 2bz·q0,   2bx·q1 + 2bz·q3,   2bx·q0 - 4bz·q2    ]
     *      ∂w/∂q3 = [ -4bx·q3 + 2bz·q1,  -2bx·q0 + 2bz·q2,   2bx·q1            ]
     *   把这三列分别与 f3/f4/f5 相乘后累加到 e0..e3。
     *
     * 三、说明
     *   这里刻意不假设 q 已归一化（C6 的雅可比就是按未归一化形式推的）；
     *   归一化统一由 IMU_Attitude_Update 在积分之后做。
     *   use_mag=0 时只保留重力 3×1 残差（地磁失效/未校准）。
     * ====================================================================== */
    float q0 = q->w, q1 = q->x, q2 = q->y, q3 = q->z;
    float f[6];
    float vx, vy, vz;
    float bx = 0.0f, bz = 0.0f;
    float e0, e1, e2, e3;

    /* ---- 重力残差 f0..f2 = v - a（v 为 C_e^b·g^e 的预测方向）---- */
    vx = 2.0f * (q1 * q3 - q0 * q2);
    vy = 2.0f * (q0 * q1 + q2 * q3);
    vz = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;

    f[0] = vx - a->x;
    f[1] = vy - a->y;
    f[2] = vz - a->z;
    f[3] = 0.0f;
    f[4] = 0.0f;
    f[5] = 0.0f;

    if (use_mag) {
        /* ---- 地磁参考 b' = [bx, 0, bz]（笔记 C7 第八章）----
         *   h = R_q·m 为地理系地磁，把水平分量长度记作 bx、垂直分量记作 bz，
         *   于是参考方向恒为 [bx,0,bz]；yaw 的零点由此固定为“磁北”。
         */
        float hx = 2.0f * m->x * (0.5f - q2 * q2 - q3 * q3)
                 + 2.0f * m->y * (q1 * q2 - q0 * q3)
                 + 2.0f * m->z * (q1 * q3 + q0 * q2);
        float hy = 2.0f * m->x * (q1 * q2 + q0 * q3)
                 + 2.0f * m->y * (0.5f - q1 * q1 - q3 * q3)
                 + 2.0f * m->z * (q2 * q3 - q0 * q1);
        float hz = 2.0f * m->x * (q1 * q3 - q0 * q2)
                 + 2.0f * m->y * (q2 * q3 + q0 * q1)
                 + 2.0f * m->z * (0.5f - q1 * q1 - q2 * q2);

        float wx, wy, wz;

        bx = sqrtf(hx * hx + hy * hy);
        bz = hz;

        /* ---- w = R_qᵀ · [bx, 0, bz] ---- */
        wx = 2.0f * bx * (0.5f - q2 * q2 - q3 * q3)
           + 2.0f * bz * (q1 * q3 - q0 * q2);
        wy = 2.0f * bx * (q1 * q2 - q0 * q3)
           + 2.0f * bz * (q0 * q1 + q2 * q3);
        wz = 2.0f * bx * (q0 * q2 + q1 * q3)
           + 2.0f * bz * (0.5f - q1 * q1 - q2 * q2);

        f[3] = wx - m->x;
        f[4] = wy - m->y;
        f[5] = wz - m->z;
    }

    /* ---- 重力部分：笔记 C6 的 Jᵀ（4×3） ---- */
    e0 = -2.0f * q2 * f[0] + 2.0f * q1 * f[1] + 2.0f * q0 * f[2];
    e1 =  2.0f * q3 * f[0] + 2.0f * q0 * f[1] - 2.0f * q1 * f[2];
    e2 = -2.0f * q0 * f[0] + 2.0f * q3 * f[1] - 2.0f * q2 * f[2];
    e3 =  2.0f * q1 * f[0] + 2.0f * q2 * f[1] + 2.0f * q3 * f[2];

    if (use_mag) {
        /* ---- 地磁部分：w 对 q 的偏导（4×3） ---- */
        e0 += -2.0f * bz * q2 * f[3]
            + ( 2.0f * bz * q1 - 2.0f * bx * q3) * f[4]
            +  2.0f * bx * q2 * f[5];

        e1 +=  2.0f * bz * q3 * f[3]
            + ( 2.0f * bx * q2 + 2.0f * bz * q0) * f[4]
            + ( 2.0f * bx * q3 - 4.0f * bz * q1) * f[5];

        e2 += (-4.0f * bx * q2 - 2.0f * bz * q0) * f[3]
            + ( 2.0f * bx * q1 + 2.0f * bz * q3) * f[4]
            + ( 2.0f * bx * q0 - 4.0f * bz * q2) * f[5];

        e3 += (-4.0f * bx * q3 + 2.0f * bz * q1) * f[3]
            + (-2.0f * bx * q0 + 2.0f * bz * q2) * f[4]
            +  2.0f * bx * q1 * f[5];
    }

    grad[0] = e0;
    grad[1] = e1;
    grad[2] = e2;
    grad[3] = e3;
}
