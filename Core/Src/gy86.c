/* USER CODE BEGIN 0 */

/**
 * @brief MPU6050 初始化 (包括旁路模式)
 */
void MPU6050_Init(void)
{
    uint8_t Data[2];
    uint8_t check;

    // 1. 检测 MPU6050
    if(HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, WHO_AM_I_REG, 1, &check, 1, 100) != HAL_OK || check != 0x68)
    {
        // 检查失败
        return;
    }

    // 2. 唤醒传感器 (PWR_MGMT_1, 0x6B -> 0x00)
    Data[0] = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, PWR_MGMT_1_REG, 1, Data, 1, 100);
    HAL_Delay(10); // 等待唤醒

    // 3. 开启 I2C 旁路模式 (INT_PIN_CFG, 0x37 -> 0x02)
    // 这是访问 HMC5883L 和 MS5611 的关键步骤
    Data[0] = 0x02; // BYPASS_EN = 1
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, INT_PIN_CFG_REG, 1, Data, 1, 100);
}

/**
 * @brief HMC5883L 初始化
 */
void HMC5883L_Init(void)
{
    uint8_t Data[2];

    // 1. 配置寄存器A (HMC_CONF_A_REG, 0x00) -> 0x70
    // 0x70: 8-average, 15Hz Data Rate, Normal Measurement Mode
    Data[0] = 0x70;
    HAL_I2C_Mem_Write(&hi2c1, HMC5883L_ADDR, HMC_CONF_A_REG, 1, Data, 1, 100);

    // 2. 模式寄存器 (HMC_MODE_REG, 0x02) -> 0x00
    // 0x00: 连续测量模式 (Continuous-Measurement Mode)
    Data[0] = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, HMC5883L_ADDR, HMC_MODE_REG, 1, Data, 1, 100);
}


/**
 * @brief MS5611 重置并读取 PROM 校准参数
 */
void MS5611_PROM_Read(void)
{
    uint8_t Data_Rx[2];
    uint8_t i;

    // 1. 发送 Reset 命令
    HAL_I2C_Master_Transmit(&hi2c1, MS5611_ADDR, (uint8_t*)MS_CMD_RESET, 1, 100);
    HAL_Delay(5); // 等待复位完成

    // 2. 读取 6 个校准字 C1 到 C6 (共 12 字节)
    for (i = 0; i < 6; i++)
    {
        uint8_t reg_addr = MS_CMD_PROM_READ_BASE + (i * 2);

        // 发送读取 PROM 地址命令 (例如 0xA2, 0xA4, ...)
        HAL_I2C_Master_Transmit(&hi2c1, MS5611_ADDR, &reg_addr, 1, 100);

        // 读取 2 字节数据
        HAL_I2C_Master_Receive(&hi2c1, MS5611_ADDR, Data_Rx, 2, 100);

        // 拼接成 16 位整数 (C[1] ~ C[6])
        C[i+1] = (uint16_t)(Data_Rx[0] << 8 | Data_Rx[1]);
    }
}


/**
 * @brief 在 main() 函数的 USER CODE BEGIN 2 区域调用所有初始化函数
 */
void All_Sensors_Init(void)
{
    MPU6050_Init();
    HMC5883L_Init();
    MS5611_PROM_Read(); // MS5611 必须在启动时读取校准数据
}

// ------------------- 读取函数 -------------------

/**
 * @brief 读取 MPU6050 原始数据 (Acc + Gyro)
 */
void MPU6050_Read_Raw(void)
{
    uint8_t Rec_Data[14]; // 从 ACCEL_XOUT_H 到 GYRO_ZOUT_L 共 14 字节

    // 一次性从 0x3B 开始连续读取 14 个字节
    if (HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, ACCEL_XOUT_H_REG, 1, Rec_Data, 14, 100) == HAL_OK)
    {
        // 加速度计 (X, Y, Z)
        Accel_X_RAW = (int16_t)(Rec_Data[0] << 8 | Rec_Data[1]);
        Accel_Y_RAW = (int16_t)(Rec_Data[2] << 8 | Rec_Data[3]);
        Accel_Z_RAW = (int16_t)(Rec_Data[4] << 8 | Rec_Data[5]);

        // 忽略温度 Rec_Data[6] 和 Rec_Data[7]

        // 陀螺仪 (X, Y, Z)
        Gyro_X_RAW = (int16_t)(Rec_Data[8] << 8 | Rec_Data[9]);
        Gyro_Y_RAW = (int16_t)(Rec_Data[10] << 8 | Rec_Data[11]);
        Gyro_Z_RAW = (int16_t)(Rec_Data[12] << 8 | Rec_Data[13]);
    }
}

/**
 * @brief 读取 HMC5883L 原始磁场数据
 */
void HMC5883L_Read_Raw(void)
{
    uint8_t Rec_Data[6];

    // 从 HMC_DATA_X_H_REG (0x03) 开始连续读取 6 个字节
    // 顺序是 X高, X低, Z高, Z低, Y高, Y低
    if (HAL_I2C_Mem_Read(&hi2c1, HMC5883L_ADDR, HMC_DATA_X_H_REG, 1, Rec_Data, 6, 100) == HAL_OK)
    {
        Mag_X_RAW = (int16_t)(Rec_Data[0] << 8 | Rec_Data[1]);
        Mag_Z_RAW = (int16_t)(Rec_Data[2] << 8 | Rec_Data[3]); // Z轴
        Mag_Y_RAW = (int16_t)(Rec_Data[4] << 8 | Rec_Data[5]); // Y轴
    }
}

/**
 * @brief 读取 MS5611 原始 D1 和 D2 数据
 * MS5611 读数需要：发送转换命令 -> 等待 -> 发送读取 ADC 命令
 */
void MS5611_Read_Raw(void)
{
    uint8_t Rx_Buf[3];

    // ----------------- 1. 读取原始压力 (D1) -----------------
    // 发送 D1 转换命令 (OSR=4096, 0x48)
    HAL_I2C_Master_Transmit(&hi2c1, MS5611_ADDR, (uint8_t*)MS_CMD_D1_4096, 1, 100);
    HAL_Delay(10); // 等待转换完成 (4096 OSR 需要 ~9.04ms)

    // 发送读取 ADC 结果命令 (0x00)
    HAL_I2C_Master_Transmit(&hi2c1, MS5611_ADDR, (uint8_t*)MS_CMD_READ_ADC, 1, 100);

    // 读取 3 字节 ADC 数据
    if (HAL_I2C_Master_Receive(&hi2c1, MS5611_ADDR, Rx_Buf, 3, 100) == HAL_OK)
    {
        D1_Pressure_RAW = (uint32_t)(Rx_Buf[0] << 16 | Rx_Buf[1] << 8 | Rx_Buf[2]);
    }

    // ----------------- 2. 读取原始温度 (D2) -----------------
    // 发送 D2 转换命令 (OSR=4096, 0x58)
    HAL_I2C_Master_Transmit(&hi2c1, MS5611_ADDR, (uint8_t*)MS_CMD_D2_4096, 1, 100);
    HAL_Delay(10); // 等待转换完成

    // 发送读取 ADC 结果命令 (0x00)
    HAL_I2C_Master_Transmit(&hi2c1, MS5611_ADDR, (uint8_t*)MS_CMD_READ_ADC, 1, 100);

    // 读取 3 字节 ADC 数据
    if (HAL_I2C_Master_Receive(&hi2c1, MS5611_ADDR, Rx_Buf, 3, 100) == HAL_OK)
    {
        D2_Temperature_RAW = (uint32_t)(Rx_Buf[0] << 16 | Rx_Buf[1] << 8 | Rx_Buf[2]);
    }
}

/* USER CODE END 0 */