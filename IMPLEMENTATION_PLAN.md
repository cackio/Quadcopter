# 四轴飞行器飞控系统实施方案

> 基于 STM32F401RE + uC/OS-II + HAL 库
> 参考：QuadcopterNotes 笔记 C1~C7 + PID

---

## 一、总体目标

在现有 Quadcopter 工程基础上，添加姿态解算、传感器校准、PID 控制、高度解算等核心功能，实现完整的四轴飞行器飞控系统。

---

## 二、缺失模块清单

| 模块 | 功能 | 笔记依据 | 当前状态 |
|------|------|---------|---------|
| 传感器校准 | 加速度计/陀螺仪/地磁计椭球拟合 | C4、C5 | ❌ 未实现 |
| 姿态解算 | 四元数初始化、Madgwick 梯度下降 | C5、C6、C7 | ❌ 未实现 |
| 高度解算 | MS5611 温度补偿、气压转高度、惯性滤波 | C7 | ❌ 未实现 |
| PID 控制 | 串级 PID、电机混控、安全保护 | PID.md、C3 | ❌ 未实现 |
| 遥控器映射 | PPM → 期望 roll/pitch/yaw/thr | C3 | ❌ 未实现 |

---

## 三、新增文件清单

### 3.1 头文件（Core/Inc）

| 文件 | 职责 | 依赖 |
|------|------|------|
| `imu.h` | 传感器校准 + 姿态解算接口 | main.h, math.h, bsp.h |
| `pid.h` | 串级 PID + 电机混控接口 | main.h, imu.h, bsp.h |
| `altitude.h` | 气压高度解算接口 | main.h, math.h, bsp.h, imu.h |

### 3.2 源文件（Core/Src）

| 文件 | 职责 | 依赖 |
|------|------|------|
| `imu.c` | 传感器校准 + 姿态解算实现 | imu.h, bsp.h, ucos_ii.h |
| `pid.c` | 串级 PID + 电机混控实现 | pid.h, imu.h, bsp.h, ucos_ii.h |
| `altitude.c` | 气压高度解算实现 | altitude.h, bsp.h, imu.h |

### 3.3 文件间依赖关系

```
bsp.c (传感器原始数据 + 电机 PWM + PPM)
   │
   ├─→ imu.c (校准 + 姿态解算)
   │      ├─ 依赖：bsp.h (原始数据)
   │      └─ 输出：euler_angle, gyro_body, quaternion
   │
   ├─→ altitude.c (温度补偿 + 高度融合)
   │      ├─ 依赖：bsp.h (MS5611 数据)
   │      ├─ 依赖：imu.h (四元数，用于加速度计坐标转换)
   │      └─ 输出：altitude.height, velocity, acceleration
   │
   └─→ pid.c (PID + 混控)
          ├─ 依赖：imu.h (euler_angle, gyro_body)
          ├─ 依赖：bsp.h (PPM_Values, Motor_SetSpeed)
          └─ 输出：motor1~4 → bsp.c Motor_SetSpeed → TIM3 PWM
```

---

## 四、各文件详细步骤

### 4.1 imu.c — 传感器校准与姿态解算

#### 校准流程（IMU_Calib_Update）

```
步骤 1：加速度计椭球拟合
  ├─ 采集 200 组静止加速度数据
  ├─ 构建线性方程组 A·Θ = Y（笔记 C4 第 2 节）
  ├─ 最小二乘求解 Θ = (AᵀA)⁻¹AᵀY
  └─ 还原 O_x/O_y/O_z 和 S_x/S_y/S_z

步骤 2：陀螺仪零偏
  ├─ 采集 200 组静止角速度数据
  └─ 简单平均 offset = (1/n) Σ gyro_raw

步骤 3：地磁计椭球拟合
  ├─ 采集 200 组旋转磁场数据
  ├─ 构建 9 参数椭球方程（笔记 C5 第二节）
  ├─ 高斯牛顿迭代求解（笔记 C5 第三节）
  ├─ 还原硬铁偏置 b = -½M⁻¹C（笔记 C5 第 110 行）
  └─ 软铁矩阵 A（楚列斯基分解，笔记 C5 第四节）

步骤 4：四元数初始化
  ├─ 由地磁计求初始 yaw = atan2(M_x, M_y)
  ├─ 构造四元数 q = [cos(yaw/2), 0, 0, sin(yaw/2)]
  └─ 置 calib_state = CALIB_DONE
```

#### 姿态解算流程（IMU_Attitude_Update）

```
步骤 1：读取传感器原始数据（由 bsp.c 完成）

步骤 2：应用校准参数
  ├─ accel_body = (raw_accel - offset) * scale / ACCEL_LSB_PER_G
  ├─ gyro_body  = (raw_gyro - offset) * DEG_TO_RAD / GYRO_LSB_PER_DPS
  └─ mag_body   = A * (raw_mag - offset)

步骤 3：四元数预测更新（陀螺仪积分）
  ├─ q̇_ω = ½ · q ⊗ ω（笔记 C5 第 81 行）
  └─ q_pred = q + q̇_ω · dt（一阶欧拉）

步骤 4：梯度下降校正（Madgwick）
  ├─ 构建残差 f = [q·g·q* - a, q·b·q* - m]（笔记 C7 第 113 行）
  ├─ 计算梯度 ∇E = Jᵀf（笔记 C7 第 154 行）
  └─ q_corrected = q_pred - β · ∇E/‖∇E‖ · dt

步骤 5：融合更新
  └─ q_new = q_pred + (q̇_ω - β · ∇E/‖∇E‖) · dt（笔记 C7 第 175 行）

步骤 6：归一化
  └─ q_new = q_new / ‖q_new‖

步骤 7：四元数转欧拉角
  ├─ roll  = atan2(2(q0q2 - q1q3), 1-2(q2²+q3²))
  ├─ pitch = asin(2(q0q1 - q2q3))
  └─ yaw   = atan2(2(q0q3 + q1q2), 1-2(q1²+q2²))
```

#### 关键公式

| 公式 | 用途 | 笔记位置 |
|------|------|---------|
| S_x²(X-O_x)² + S_y²(Y-O_y)² + S_z²(Z-O_z)² = 1 | 加速度计校准 | C4 第 2 节 |
| (m-b)ᵀM(m-b) = 1 | 地磁计校准 | C5 第 70 行 |
| b = -½M⁻¹C | 硬铁偏置 | C5 第 108 行 |
| q̇ = ½q⊗ω | 四元数运动学 | C5 第 81 行 |
| q_t = q_{t-1} + (q̇_ω - β·∇E/‖∇E‖)·Δt | Madgwick 更新 | C7 第 175 行 |
| θ = arcsin(2ac + 2bd) | 四元数转欧拉角 | C2 第 88 行 |

---

### 4.2 pid.c — 串级 PID 控制与电机混控

#### 遥控器期望值映射（PID_Exp_Calc）

```
步骤 1：读取 PPM_Values（关中断保护）
步骤 2：限幅到有效范围 [1000, 2000]
步骤 3：转换为期望值
  ├─ expRoll  = (PPM[ROLL] - 1500) * 0.03f  (最大 20°)
  ├─ expPitch = (PPM[PITCH] - 1500) * 0.04f  (最大 20°)
  ├─ expYaw   = (PPM[YAW] - 1500) * 0.02f  (最大 10°/s)
  └─ expThr   = PPM[THR]
步骤 4：转换单位（角度 → 弧度）
```

#### 串级 PID 计算（PID_Cascade_Calc）

```
步骤 1：计算外环误差 e = shell_err
步骤 2：外环输出 = Kp * e
步骤 3：计算内环误差 e = 外环输出 - core_status
步骤 4：内环积分分离限幅
步骤 5：内环输出 = Kp*(e + eSum*Ki + (e-eK_1)*Kd)
步骤 6：内环输出限幅
```

#### 电机混控（PID_Motor_Calc）

```
步骤 1：计算 Roll/Pitch/Yaw PID 输出
步骤 2：按混控公式计算 4 路电机
  ├─ motor1 = thr - pitch + roll - yaw
  ├─ motor2 = thr - pitch - roll + yaw
  ├─ motor3 = thr + pitch + roll + yaw
  └─ motor4 = thr + pitch - roll - yaw
步骤 3：限幅到 [PWM_OUT_MIN, PWM_OUT_MAX]
步骤 4：安全保护检查
```

#### 安全保护（PID_Safety_Check）

```
条件检查：
  ├─ 油门 < THR_MIN → 停转
  ├─ |roll| > ANGLE_LIMIT_DEG → 停转
  ├─ |pitch| > ANGLE_LIMIT_DEG → 停转
  └─ 未解锁 → 停转
```

#### 飞行模式状态机（PID_FlyMode_Update）

```
STOP → HOVER: exp_thr > THR_ARM (1350)
HOVER → UP: exp_thr > 1650
HOVER → DOWN: exp_thr < THR_ARM
DOWN → STOP: exp_thr < THR_DISARM (1050)
DOWN → HOVER: exp_thr > THR_ARM
UP → HOVER: exp_thr < 1650
```

#### 关键公式

| 公式 | 用途 | 笔记位置 |
|------|------|---------|
| M = Kp·e + Ki·∫e dτ + Kd·de/dt | PID 控制律 | PID.md 第 720 行 |
| ωn = √(Kp/I) | 自然频率 | PID.md 第 190 行 |
| ζ = Kd/(2√(Kp·I)) | 阻尼比 | PID.md 第 230 行 |
| Ki < Kd·Kp/I | 稳定性条件 | PID.md 第 560 行 |
| motor1 = thr - pitch + roll - yaw | 混控公式 | C3 第 193 行 |

---

### 4.3 altitude.c — 气压高度解算

#### 校准流程（ALT_Calib_Update）

```
步骤 1：读取 MS5611 原始数据
步骤 2：温度补偿
  ├─ dT = D2 - (C5 << 8)
  ├─ T = 2000 + dT * C6 / 0x800000
  └─ 二阶补偿（T < 20°C 时）
步骤 3：气压计算
  ├─ OFF = C2<<16 + C4*dT/0x80
  ├─ SENS = C1<<15 + C3*dT/0x100
  └─ pressure = (D1*SENS/0x200000 - OFF) / 0x8000
步骤 4：累加气压值
步骤 5：采样满后计算零偏 offset_press
步骤 6：计算 K_press_to_high
  └─ K = -44300 * (offset/101325)^(-4.256/5.256) / (5.256*101325)
```

#### 高度解算流程（ALT_Update）

```
步骤 1：读取 MS5611 原始数据
步骤 2：温度补偿计算
步骤 3：气压计算
步骤 4：气压转高度
  └─ baro_height = (pressure - offset_press) * K_press_to_high
步骤 5：加速度计高度融合
  ├─ 将加速度从机体系转到地理系
  ├─ 扣除重力加速度
  └─ 惯性滤波器融合
步骤 6：更新 altitude 结构体
```

#### 惯性滤波器（ALT_Inertial_Filter）

```
预测：
  ├─ z[1] += z[2] * dt
  └─ z[0] += z[1] * dt + 0.5 * z[2] * dt²

校正：
  ├─ ewdt = error * weight * dt
  ├─ z[index] += ewdt
  └─ 关联分量更新
```

#### 关键公式

| 公式 | 用途 | 笔记位置 |
|------|------|---------|
| T = 2000 + dT*C6/0x800000 | 温度补偿 | 学长 GY86.c |
| p = (D1*SENS/0x200000 - OFF)/0x8000 | 气压计算 | 学长 GY86.c |
| h = (p - p0) * K | 气压转高度 | 学长 IMU.c |
| K = -44300*(p0/101325)^(-4.256/5.256)/(5.256*101325) | 高度系数 | 学长 IMU.c |

---

## 五、任务结构调整

### 5.1 当前任务（4 个）

| 任务 | 优先级 | 周期 | 功能 |
|------|--------|------|------|
| StartTask | 5 | 一次性 | BSP_Init + 创建子任务 |
| MotorTask | 7 | 20ms | 4 路同步油门 |
| SensorTask | 8 | 50ms | 传感器轮询 |
| UartTask | 9 | 500ms | 串口打印 |

### 5.2 调整后任务（6 个）

| 任务 | 优先级 | 周期 | 功能 |
|------|--------|------|------|
| StartTask | 5 | 一次性 | BSP_Init + 校准启动 + 创建子任务 |
| AngelTask | 7 | 10ms | 传感器读取 + 姿态解算 |
| HeightTask | 8 | 10ms | MS5611 读取 + 高度解算 |
| PIDTask | 6 | 20ms | 遥控器映射 + PID + 电机输出 |
| UartTask | 9 | 500ms | 串口打印（保留） |

### 5.3 任务合并说明

- **SensorTask → AngelTask**：传感器读取和解算合并，避免 I2C 争用
- **MotorTask → PIDTask**：PID 和电机输出合并，保证控制链时序一致

---

## 六、main.c 修改要点

### 6.1 新增任务定义

```c
#define ANGEL_TASK_PRIO       7
#define HEIGHT_TASK_PRIO      8
#define PID_TASK_PRIO         6

#define ANGEL_TASK_STK_SIZE   512
#define HEIGHT_TASK_STK_SIZE  512
#define PID_TASK_STK_SIZE     512

OS_STK AngelTaskStk[ANGEL_TASK_STK_SIZE];
OS_STK HeightTaskStk[HEIGHT_TASK_STK_SIZE];
OS_STK PidTaskStk[PID_TASK_STK_SIZE];
```

### 6.2 任务函数

```c
void AngelTask(void *p_arg)
{
    while (1) {
        MPU6050_Read_Raw();
        HMC5883L_Read_Raw();

        if (IMU_Calib_Done()) {
            IMU_Attitude_Update();
        } else {
            IMU_Calib_Update();
        }

        OSTimeDlyHMSM(0, 0, 0, 10);
    }
}

void HeightTask(void *p_arg)
{
    while (1) {
        MS5611_Read_Raw();

        if (ALT_Calib_Done()) {
            ALT_Update();
        } else {
            ALT_Calib_Update();
        }

        OSTimeDlyHMSM(0, 0, 0, 10);
    }
}

void PidTask(void *p_arg)
{
    while (1) {
        PID_Exp_Calc();

        if (IMU_Calib_Done() && ALT_Calib_Done()) {
            PID_Motor_Calc();
            PID_Motor_Output();
        }

        OSTimeDlyHMSM(0, 0, 0, 20);
    }
}
```

### 6.3 StartTask 修改

```c
void StartTask(void *p_arg)
{
    BSP_Init();
    OS_TRACE_INIT();

    /* 初始化 PID 和高度模块 */
    PID_Init();
    PID_Time_Init();
    ALT_Init();
    IMU_Calib_Init();

    /* 创建子任务 */
    OSTaskCreate(AngelTask, ...);
    OSTaskCreate(HeightTask, ...);
    OSTaskCreate(PidTask, ...);
    OSTaskCreate(UartTask, ...);

    OSTaskDel(OS_PRIO_SELF);
}
```

---

## 七、bsp.c 修改要点

### 7.1 MPU6050_Init 补充量程配置

```c
void MPU6050_Init(void)
{
    // ... 现有代码 ...

    /* 配置陀螺仪量程 ±2000°/s */
    Data[0] = 0x18;  // GYRO_CONFIG
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, 0x1B, 1, Data, 1, 100);

    /* 配置加速度计量程 ±4g */
    Data[0] = 0x08;  // ACCEL_CONFIG
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, 0x1C, 1, Data, 1, 100);
}
```

### 7.2 暴露 MS5611 数据

MS5611 的原始数据 `D1_Pressure_RAW`、`D2_Temperature_RAW` 和 PROM 校准字 `C[7]` 已在 bsp.c 定义，只需在 altitude.c 中通过 `extern` 引用（已在 altitude.h 声明）。

---

## 八、与学长工程的关键差异

| 维度 | 学长 | 你的 Quadcopter |
|------|------|-----------------|
| I2C | 软件模拟 | **HAL 硬件 I2C1** |
| 库 | 标准外设库 | **HAL 库** |
| 传感器校准 | 简单平均 | **椭球拟合**（更精确） |
| 姿态解算 | Mahony | **Madgwick**（更优） |
| 遥控输入 | TIM5 四路 | **TIM2 单路 PPM** |
| 电机布局 | X 型 | **待定**（+型 或 X型） |

---

## 九、实施顺序建议

### 阶段 1：基础搭建（1-2 天）
1. 创建 6 个新文件（已完成）
2. 修改 main.c 任务结构
3. 修改 bsp.c 补充配置
4. 编译验证

### 阶段 2：传感器校准（2-3 天）
1. 实现加速度计椭球拟合
2. 实现陀螺仪零偏
3. 实现地磁计椭球拟合
4. 联调校准流程

### 阶段 3：姿态解算（2-3 天）
1. 实现四元数初始化
2. 实现 Madgwick 梯度下降
3. 实现四元数转欧拉角
4. 联调姿态解算

### 阶段 4：高度解算（1-2 天）
1. 实现 MS5611 温度补偿
2. 实现气压转高度
3. 实现惯性滤波器
4. 联调高度解算

### 阶段 5：PID 控制（2-3 天）
1. 实现遥控器期望值映射
2. 实现串级 PID
3. 实现电机混控
4. 实现安全保护
5. 联调控制流程

### 阶段 6：整体调试（3-5 天）
1. 传感器数据采集验证
2. 姿态解算精度验证
3. PID 参数调优
4. 飞行测试

---

## 十、注意事项

1. **坐标系符号**：务必统一笔记 C1 的 ZYX 外旋约定和笔记 C3 的电机布局定义
2. **I2C 争用**：HAL 硬件 I2C 不支持多主机，校准采集时需串行化
3. **采样周期**：imu_dt 和 pid_dt 必须精确测量，影响积分精度
4. **安全保护**：倾斜角保护和信号丢失保护必须优先于 PID 输出
5. **电机布局**：混控公式中的符号需根据实际电机转向调整
6. **校准数据量**：椭球拟合需要足够多的采样点（建议 ≥200 组）
