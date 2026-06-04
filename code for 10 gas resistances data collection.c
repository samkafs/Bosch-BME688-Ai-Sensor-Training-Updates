/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "NanoEdgeAI.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */
/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    // Temperature
    uint16_t par_t1;
    int16_t  par_t2;
    int8_t   par_t3;
    // Pressure
    uint16_t par_p1;
    int16_t  par_p2;
    int8_t   par_p3;
    int16_t  par_p4;
    int16_t  par_p5;
    int8_t   par_p6;
    int8_t   par_p7;
    int16_t  par_p8;
    int16_t  par_p9;
    uint8_t  par_p10;
    // Humidity
    uint16_t par_h1;
    uint16_t par_h2;
    int8_t   par_h3;
    int8_t   par_h4;
    int8_t   par_h5;
    uint8_t  par_h6;
    int8_t   par_h7;
    // Gas Resistance
    int8_t   par_g1;
    int16_t  par_g2;
    int8_t   par_g3;
    uint8_t  res_heat_range;
    int8_t   res_heat_val;
    // Global temperature fine value (required for P/H calc)
    int32_t  t_fine;
} BME688_Calib;
typedef struct {
    float temperature;
    float pressure;
    float humidity;
    float gas_resistance;
} BME688_Data;
/* USER CODE END PTD */
/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* BME688 Device Definitions */
#define BME688_ADDR 0x76 << 1
#define REG_ID      0xD0
#define REG_RESET   0xE0
#define REG_CTRL_HUM 0x72
#define REG_CTRL_MEAS 0x74
#define REG_CONFIG  0x75
#define REG_CTRL_GAS_1 0x71
#define REG_CTRL_GAS_0 0x70
/* BME688 Data Registers */
#define REG_PRESS_MSB 0x1F
#define REG_TEMP_MSB  0x22
#define REG_HUM_MSB   0x25
#define REG_GAS_WAIT_0 0x64
#define REG_RES_HEAT_0 0x5A
#define REG_GAS_MSB    0x2C
/* USER CODE END PD */
/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */
/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;
/* USER CODE BEGIN PV */
extern UART_HandleTypeDef huart1;
static BME688_Calib calib;
enum neai_state neai_status;
int id_class;
// NanoEdge expects exactly 13 features (10 resistances + 1 humidity + 1 temperature + 1 pressure)
float input_signal[13];
float probabilities[NEAI_NUMBER_OF_CLASSES];
/* USER CODE END PV */
/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void BME688_Init(I2C_HandleTypeDef *hi2c);
uint8_t BME688_CalcHeaterRes(uint16_t target_temp, uint16_t amb_temp);
void BME688_TriggerMeasurement(I2C_HandleTypeDef *hi2c);
int32_t BME688_ReadTemperature(I2C_HandleTypeDef *hi2c, int field);
int32_t BME688_ReadPressure(I2C_HandleTypeDef *hi2c, int field);
int32_t BME688_ReadHumidity(I2C_HandleTypeDef *hi2c, int field);
int32_t BME688_ReadGasResistance(I2C_HandleTypeDef *hi2c, int field);
int BME688_GetNewestField(I2C_HandleTypeDef *hi2c);
// NEW Function Prototype for running the optimized 10-step gas scan
void BME688_RunGasScan(I2C_HandleTypeDef *hi2c, int32_t *scan_results, int32_t *temp_out, int32_t *hum_out, int32_t *pres_out);
/* USER CODE END PFP */
/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
/* USER CODE END 0 */
void BME688_Init(I2C_HandleTypeDef *hi2c) {
    uint8_t coeff_array1[25]; // 0x89 to 0xA1
    uint8_t coeff_array2[16]; // 0xE1 to 0xF0
    uint8_t heat_calib[3];    // 0x00 to 0x02
    // Read calibration data
    HAL_I2C_Mem_Read(hi2c, BME688_ADDR, 0x89, 1, coeff_array1, 25, 100);
    HAL_I2C_Mem_Read(hi2c, BME688_ADDR, 0xE1, 1, coeff_array2, 16, 100);
    HAL_I2C_Mem_Read(hi2c, BME688_ADDR, 0x00, 1, heat_calib, 3, 100);
    // Parse Temperature Calib
    calib.par_t1 = (uint16_t)((coeff_array2[9] << 8) | coeff_array2[8]);
    calib.par_t2 = (int16_t)((coeff_array1[2] << 8) | coeff_array1[1]);
    calib.par_t3 = (int8_t)(coeff_array1[3]);
    // Parse Pressure Calib
    calib.par_p1 = (uint16_t)((coeff_array1[6] << 8) | coeff_array1[5]);
    calib.par_p2 = (int16_t)((coeff_array1[8] << 8) | coeff_array1[7]);
    calib.par_p3 = (int8_t)(coeff_array1[9]);
    calib.par_p4 = (int16_t)((coeff_array1[12] << 8) | coeff_array1[11]);
    calib.par_p5 = (int16_t)((coeff_array1[14] << 8) | coeff_array1[13]);
    calib.par_p6 = (int8_t)(coeff_array1[16]);
    calib.par_p7 = (int8_t)(coeff_array1[15]);
    calib.par_p8 = (int16_t)((coeff_array1[20] << 8) | coeff_array1[19]);
    calib.par_p9 = (int16_t)((coeff_array1[22] << 8) | coeff_array1[21]);
    calib.par_p10 = (uint8_t)(coeff_array1[23]);
    // Parse Humidity Calib
    calib.par_h1 = (uint16_t)(((uint16_t)coeff_array2[2] << 4) | (coeff_array2[1] & 0x0F));
    calib.par_h2 = (uint16_t)(((uint16_t)coeff_array2[0] << 4) | ((coeff_array2[1] & 0xF0) >> 4));
    calib.par_h3 = (int8_t)coeff_array2[3];
    calib.par_h4 = (int8_t)coeff_array2[4];
    calib.par_h5 = (int8_t)coeff_array2[5];
    calib.par_h6 = (uint8_t)coeff_array2[6];
    calib.par_h7 = (int8_t)coeff_array2[7];
    // Parse Gas Calib
    calib.par_g1 = (int8_t)coeff_array2[12];
    calib.par_g2 = (int16_t)((coeff_array2[11] << 8) | coeff_array2[10]);
    calib.par_g3 = (int8_t)coeff_array2[13];
    calib.res_heat_val = (int8_t)heat_calib[0];
    calib.res_heat_range = (uint8_t)((heat_calib[2] >> 4) & 0x03);
}
uint8_t BME688_CalcHeaterRes(uint16_t target_temp, uint16_t amb_temp) {
    if (target_temp > 400) target_temp = 400;
    int32_t var1, var2, var3, var4, var5;
    int32_t res_heat_x100;
    uint8_t res_heat_x;
    var1 = (((int32_t)amb_temp * (int32_t)calib.par_g3) / 10) << 8;
    var2 = ((int32_t)calib.par_g1 + 784) *
           ((((((int32_t)calib.par_g2 + 154009) * (int32_t)target_temp * 5) / 100) + 3276800) / 10);
    var3 = var1 + (var2 >> 1);
    var4 = var3 / ((int32_t)calib.res_heat_range + 4);
    var5 = (131 * (int32_t)calib.res_heat_val) + 65536;
    res_heat_x100 = (((var4 / var5) - 250) * 34);
    res_heat_x = (uint8_t)((res_heat_x100 + 50) / 100);
    if (res_heat_x > 255) res_heat_x = 255;
    return res_heat_x;
}
void BME688_TriggerMeasurement(I2C_HandleTypeDef *hi2c) {
    uint8_t osrs_h = 0x01;
    HAL_I2C_Mem_Write(hi2c, BME688_ADDR, REG_CTRL_HUM, 1, &osrs_h, 1, 100);
    uint8_t res_heat = BME688_CalcHeaterRes(320, 25);
    HAL_I2C_Mem_Write(hi2c, BME688_ADDR, REG_RES_HEAT_0, 1, &res_heat, 1, 100);
    uint8_t gas_wait;
    uint16_t duration_ms = 250;
    if (duration_ms <= 63) {
        gas_wait = duration_ms;
    } else if (duration_ms <= 252) {
        gas_wait = 0x40 | ((duration_ms + 2) / 4);
    } else if (duration_ms <= 1008) {
        gas_wait = 0x80 | ((duration_ms + 8) / 16);
    } else {
        gas_wait = 0xC0 | ((duration_ms + 32) / 64);
    }
    HAL_I2C_Mem_Write(hi2c, BME688_ADDR, REG_GAS_WAIT_0, 1, &gas_wait, 1, 100);
    uint8_t ctrl_gas = 0x20;
    HAL_I2C_Mem_Write(hi2c, BME688_ADDR, REG_CTRL_GAS_1, 1, &ctrl_gas, 1, 100);
    uint8_t ctrl_meas = 0x55;
    HAL_I2C_Mem_Write(hi2c, BME688_ADDR, REG_CTRL_MEAS, 1, &ctrl_meas, 1, 100);
}
/* Helper to check which of the 3 fields has fresh data (checks bit 7: new_data) */
int BME688_GetNewestField(I2C_HandleTypeDef *hi2c) {
    uint8_t status;

    // Check Field 0 (Status Register: 0x1D)
    if (HAL_I2C_Mem_Read(hi2c, BME688_ADDR, 0x1D, 1, &status, 1, 100) == HAL_OK) {
        if (status & 0x80) { // new_data flag set
            return 0;
        }
    }
    // Check Field 1 (Status Register: 0x2F)
    if (HAL_I2C_Mem_Read(hi2c, BME688_ADDR, 0x2F, 1, &status, 1, 100) == HAL_OK) {
        if (status & 0x80) { // new_data flag set
            return 1;
        }
    }
    // Check Field 2 (Status Register: 0x41)
    if (HAL_I2C_Mem_Read(hi2c, BME688_ADDR, 0x41, 1, &status, 1, 100) == HAL_OK) {
        if (status & 0x80) { // new_data flag set
            return 2;
        }
    }
    return -1; // No new data available yet
}
int32_t BME688_ReadTemperature(I2C_HandleTypeDef *hi2c, int field) {
    uint8_t temp_data[3];
    uint8_t reg_addr = 0x22 + (field * 18); // Apply 18-byte field offset
    HAL_I2C_Mem_Read(hi2c, BME688_ADDR, reg_addr, 1, temp_data, 3, 100);
    uint32_t adc_t = (uint32_t)(((uint32_t)temp_data[0] << 12) |
                                 ((uint32_t)temp_data[1] << 4) |
                                 ((uint32_t)temp_data[2] >> 4));
    int64_t var1, var2, var3;
    var1 = ((int32_t)adc_t >> 3) - ((int32_t)calib.par_t1 << 1);
    var2 = (var1 * (int32_t)calib.par_t2) >> 11;
    var3 = ((((var1 >> 1) * (var1 >> 1)) >> 12) * ((int32_t)calib.par_t3 << 4)) >> 14;
    calib.t_fine = (int32_t)(var2 + var3);
    int32_t temperature = ((calib.t_fine * 5) + 128) >> 8;
    return temperature;
}
int32_t BME688_ReadPressure(I2C_HandleTypeDef *hi2c, int field) {
    uint8_t press_data[3];
    uint8_t reg_addr = 0x1F + (field * 18); // Apply 18-byte field offset
    HAL_I2C_Mem_Read(hi2c, BME688_ADDR, reg_addr, 1, press_data, 3, 100);
    uint32_t adc_p = (uint32_t)(((uint32_t)press_data[0] << 12) |
                                 ((uint32_t)press_data[1] << 4) |
                                 ((uint32_t)press_data[2] >> 4));
    int64_t var1, var2;
    var1 = ((int64_t)calib.t_fine >> 1) - 64000;
    var2 = ((((var1 >> 2) * (var1 >> 2)) >> 11) * (int64_t)calib.par_p6) >> 2;
    var2 = var2 + ((var1 * (int64_t)calib.par_p5) << 1);
    var2 = (var2 >> 2) + ((int64_t)calib.par_p4 << 16);
    var1 = (((((var1 >> 2) * (var1 >> 2)) >> 13) * ((int64_t)calib.par_p3 << 5)) >> 3) +
           (((int64_t)calib.par_p2 * var1) >> 1);
    var1 = var1 >> 18;
    var1 = ((32768 + var1) * (int64_t)calib.par_p1) >> 15;
    if (var1 == 0) return 0;
    uint32_t calc_pres = (uint32_t)(((uint32_t)(1048576 - adc_p) - (var2 >> 12)) * 3125);
    if (calc_pres < 0x80000000)
        calc_pres = (calc_pres << 1) / ((uint32_t)var1);
    else
        calc_pres = (calc_pres / (uint32_t)var1) * 2;
    var1 = ((int64_t)calib.par_p9 * (int64_t)(((calc_pres >> 3) * (calc_pres >> 3)) >> 13)) >> 12;
    var2 = ((int64_t)(calc_pres >> 2) * (int64_t)calib.par_p8) >> 13;
    int32_t var3_p = ((int32_t)(calc_pres >> 8) * (int32_t)(calc_pres >> 8) *
                      (int32_t)(calc_pres >> 8) * (int32_t)calib.par_p10) >> 17;
    calc_pres = (int32_t)(calc_pres) + ((var1 + var2 + var3_p + ((int64_t)calib.par_p7 << 7)) >> 4);
    return (int32_t)calc_pres;
}
int32_t BME688_ReadHumidity(I2C_HandleTypeDef *hi2c, int field) {
    uint8_t hum_data[2];
    uint8_t reg_addr = 0x25 + (field * 18); // Apply 18-byte field offset
    HAL_I2C_Mem_Read(hi2c, BME688_ADDR, reg_addr, 1, hum_data, 2, 100);
    uint16_t adc_h = (uint16_t)(((uint32_t)hum_data[0] << 8) | (uint32_t)hum_data[1]);
    int32_t var1, var2, var3, var4, var5, var6;
    int32_t temp_scaled;
    temp_scaled = (((int32_t)calib.t_fine * 5) + 128) >> 8;
    var1 = (int32_t)adc_h - ((int32_t)((int32_t)calib.par_h1 * 16)) -
           (((temp_scaled * (int32_t)calib.par_h3) / ((int32_t)100)) >> 1);
    var2 = ((int32_t)calib.par_h2 *
           (((temp_scaled * (int32_t)calib.par_h4) / ((int32_t)100)) +
           (((temp_scaled * ((temp_scaled * (int32_t)calib.par_h5) / ((int32_t)100))) >> 6) /
           ((int32_t)100)) + (int32_t)(1 << 14))) >> 10;
    var3 = var1 * var2;
    var4 = (int32_t)calib.par_h6 << 7;
    var4 = ((var4) + ((temp_scaled * (int32_t)calib.par_h7) / ((int32_t)100))) >> 4;
    var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
    var6 = (var4 * var5) >> 1;
    int32_t calc_hum = (((var3 + var6) >> 10) * ((int32_t)1000)) >> 12;
    if (calc_hum > 100000) calc_hum = 100000;
    if (calc_hum < 0) calc_hum = 0;
    return calc_hum;
}
int32_t BME688_ReadGasResistance(I2C_HandleTypeDef *hi2c, int field) {
    uint8_t gas_data[2];
    uint8_t reg_addr = 0x2C + (field * 18); // Apply 18-byte field offset
    HAL_I2C_Mem_Read(hi2c, BME688_ADDR, reg_addr, 1, gas_data, 2, 100);
    uint8_t gas_msb = gas_data[0];
    uint8_t gas_lsb = gas_data[1];
    int gas_valid = (gas_lsb & 0x20);
    int heat_stab = (gas_lsb & 0x10);
    if (!gas_valid || !heat_stab) {
        return 0;
    }
    uint16_t gas_adc = ((uint16_t)gas_msb << 2) | ((uint16_t)(gas_lsb >> 6) & 0x03);
    uint8_t gas_range = gas_lsb & 0x0F;
    uint32_t var1 = 262144UL >> gas_range;
    int32_t var2 = (int32_t)gas_adc - 512;
    var2 = var2 * 3 + 4096;
    if (var2 != 0) {
        uint32_t gas_res = (10000UL * var1) / (uint32_t)var2;
        gas_res = gas_res * 100;
        return (int32_t)gas_res;
    }
    return 0;
}
// Runs a 10-temperature gas scan sequentially using dynamic hardware buffers, gathering pressure as well
void BME688_RunGasScan(I2C_HandleTypeDef *hi2c, int32_t *scan_results, int32_t *temp_out, int32_t *hum_out, int32_t *pres_out) {
    // 1. Enable humidity oversampling (1x)
    uint8_t osrs_h = 0x01;
    HAL_I2C_Mem_Write(hi2c, BME688_ADDR, REG_CTRL_HUM, 1, &osrs_h, 1, 100);
    // 2. Read ambient temperature to calibrate the heater calculations
    uint8_t ctrl_meas_init = 0x55; // Trigger forced mode read
    HAL_I2C_Mem_Write(hi2c, BME688_ADDR, REG_CTRL_MEAS, 1, &ctrl_meas_init, 1, 100);
    HAL_Delay(30);
    int amb_field = -1;
    for (int retry = 0; retry < 5; retry++) {
        amb_field = BME688_GetNewestField(hi2c);
        if (amb_field != -1) {
            break;
        }
        HAL_Delay(5);
    }
    if (amb_field == -1) {
        amb_field = 0; // Fallback
    }
    int32_t raw_temp = BME688_ReadTemperature(hi2c, amb_field);
    int32_t amb_temp = raw_temp / 100;
    if (amb_temp < 10 || amb_temp > 50) {
        amb_temp = 25; // Safe default fallback
    }
    // 3. Define the 10-step heater profile (100ms duration per step)
    uint16_t heater_temps[10] = {200, 220, 240, 260, 280, 300, 320, 340, 360, 380};
    uint16_t duration_ms = 100; // 100ms per step
    int last_field = 0;
    for (int i = 0; i < 10; i++) {
        // Calculate and set heater resistance for this step (slot 0)
        uint8_t res_heat = BME688_CalcHeaterRes(heater_temps[i], (uint16_t)amb_temp);
        HAL_I2C_Mem_Write(hi2c, BME688_ADDR, REG_RES_HEAT_0, 1, &res_heat, 1, 100);
        // Convert 100ms duration to wait_0 format
        uint8_t gas_wait = 0x40 | ((duration_ms + 2) / 4);
        HAL_I2C_Mem_Write(hi2c, BME688_ADDR, REG_GAS_WAIT_0, 1, &gas_wait, 1, 100);
        // Enable gas, select step 0 (run_gas = 1, nb_conv = 0)
        uint8_t ctrl_gas = 0x20;
        HAL_I2C_Mem_Write(hi2c, BME688_ADDR, REG_CTRL_GAS_1, 1, &ctrl_gas, 1, 100);
        // Trigger measurement (Forced Mode, osrs_t=2x, osrs_p=16x)
        uint8_t ctrl_meas = 0x55;
        HAL_I2C_Mem_Write(hi2c, BME688_ADDR, REG_CTRL_MEAS, 1, &ctrl_meas, 1, 100);
        // Wait for measurement to complete (100ms heater duration + 20ms sensor readout overhead)
        HAL_Delay(duration_ms + 20);

        // Dynamically find and read the newest hardware field containing this step
        int32_t raw_gas = 0;
        int step_field = -1;
        for (int retry = 0; retry < 5; retry++) {
            step_field = BME688_GetNewestField(hi2c);
            if (step_field != -1) {
                raw_gas = BME688_ReadGasResistance(hi2c, step_field);
                if (raw_gas > 0) {
                    last_field = step_field; // Keep track of the active field
                    break;
                }
            }
            HAL_Delay(8); // Retry delay
        }
        scan_results[i] = raw_gas;
    }
    // 4. Read final environmental values after the scan using the last active field
    *temp_out = BME688_ReadTemperature(hi2c, last_field); // centidegrees (e.g. 2500)
    *pres_out = BME688_ReadPressure(hi2c, last_field);    // Pa (e.g. 101300)
    *hum_out  = BME688_ReadHumidity(hi2c, last_field);    // millipercent (e.g. 52200)
}
/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */
  /* MCU Configuration--------------------------------------------------------*/
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  /* USER CODE BEGIN Init */
  /* USER CODE END Init */
  /* Configure the system clock */
  SystemClock_Config();
  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();
  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */
  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
    // 1. Initialize the sensor
      BME688_Init(&hi2c1);
      neai_status = neai_classification_init();
//      if(neai_status != NEAI_OK)
//      {
//          printf("NanoEdge Init Failed\r\n");
//          Error_Handler();
//      }
//      else
//      {
//          printf("NanoEdge Init Success\r\n");
//      }
      // 2. Trigger the very first measurement so it is ready for the BLE timer
      BME688_TriggerMeasurement(&hi2c1);
    /* USER CODE END 2 */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
//      // Fast 32-bit integer registers for sensor readings
      int32_t gas_scan[10] = {0};
      int32_t temperature = 0;
      int32_t humidity = 0;
      int32_t pressure = 0;
      // 1. Run the 10-step gas scanning profile entirely in the integer domain (takes ~1.2s)
      BME688_RunGasScan(&hi2c1, gas_scan, &temperature, &humidity, &pressure);
      // 2. PRINT CSV DATA FOR DATASET COLLECTION
      // Simply capture this exact line from the Serial Monitor to copy-paste into CSV files
      // format: R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,Humidity,Temperature,Pressure
      printf("%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld.%02ld,%ld.%02ld,%ld\r\n",
          gas_scan[0], gas_scan[1], gas_scan[2], gas_scan[3], gas_scan[4],
          gas_scan[5], gas_scan[6], gas_scan[7], gas_scan[8], gas_scan[9],
          humidity / 1000,            // Integer part of humidity (%)
          (humidity % 1000) / 10,     // Fractional part of humidity (1 decimal place)
          temperature / 100,          // Integer part of temperature (°C)
          temperature % 100,          // Fractional part of temperature (2 decimal places)
          pressure / 100             // Integer part of pressure (hPa)
      );

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK4|RCC_CLOCKTYPE_HCLK2
                              |RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK2Divider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK4Divider = RCC_SYSCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SMPS;
  PeriphClkInitStruct.SmpsClockSelection = RCC_SMPSCLKSOURCE_HSI;
  PeriphClkInitStruct.SmpsDivSelection = RCC_SMPSCLKDIV_RANGE0;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN Smps */

  /* USER CODE END Smps */
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00000003;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pins : USB_DM_Pin USB_DP_Pin */
  GPIO_InitStruct.Pin = USB_DM_Pin|USB_DP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF10_USB;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
