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
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "fft.h"  // 包含自定义的 FFT 头文件
#include <math.h> // 包含数学库 (如果需要 M_PI 或其他数学函数，例如窗口函数)
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#define ADC_BUFFER_SIZE FFT_N // 假设 ADC 采样点数与 FFT 点数相同
// --- 模拟信号参数 ---
#define SAMPLING_FREQ 48000.0f // 假设的采样频率 (Hz) - 需要与实际 ADC 采样率匹配
#define SIGNAL_FREQ 1000.0f    // 要生成的正弦波频率 (Hz)
#define SIGNAL_AMPLITUDE 1.0f  // 正弦波幅度 (可以根据 ADC 范围调整)
#define SIGNAL_OFFSET 0.0f     // 正弦波直流偏移 (可以根据 ADC 范围调整)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

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

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  float adc_samples[ADC_BUFFER_SIZE]; // 存储原始 ADC 采样数据的数组 (假设已被填充)
  complex_t fft_input_output[FFT_N];  // FFT 输入/输出缓冲区 (复数形式)
  float fft_magnitudes[FFT_N / 2];    // 存储 FFT 幅度结果的数组

  // --- 假设 adc_samples 数组已经填充了 ADC 采集的数据 ---
  // 例如: 通过 ADC DMA 传输填充

  // --- 生成模拟正弦波信号 ---
  for (int i = 0; i < ADC_BUFFER_SIZE; i++)
  {
    // 计算当前采样点的时间 t = i / SAMPLING_FREQ
    // 计算正弦波值: A * sin(2 * pi * f * t) + offset
    adc_samples[i] = SIGNAL_AMPLITUDE * sinf(2.0f * M_PI * SIGNAL_FREQ * (float)i / SAMPLING_FREQ) + SIGNAL_OFFSET;
  }
  // --- 模拟信号生成结束 ---

  // 准备 FFT 输入缓冲区
  for (int i = 0; i < FFT_N; i++)
  {
    if (i < ADC_BUFFER_SIZE) // 检查是否在有效 ADC 数据范围内
    {
      // 可选：在此处应用窗口函数 (例如汉宁窗) 以减少频谱泄漏
      // float window_coeff = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FFT_N - 1)));
      // fft_input_output[i].real = adc_samples[i] * window_coeff; // 应用窗口
      fft_input_output[i].real = adc_samples[i]; // 将 ADC 采样值放入实部 (无窗口示例)
    }
    else
    {
      // 如果 ADC 采样点数少于 FFT 点数，进行零填充
      fft_input_output[i].real = 0.0f;
    }
    fft_input_output[i].imag = 0.0f; // 虚部初始化为 0
  }

  // 执行 FFT 计算 (原地操作，结果覆盖 fft_input_output)
  fft_radix2(fft_input_output, FFT_N);

  // 计算 FFT 结果的幅度
  fft_calculate_magnitudes(fft_input_output, fft_magnitudes, FFT_N);

  // 现在 fft_magnitudes 数组包含了频域的幅度信息
  // 可以对 fft_magnitudes 进行处理，例如查找峰值频率
  float max_magnitude = 0; // 最大幅度值
  uint32_t max_index = 0;  // 最大幅度对应的索引
  // 从索引 1 开始查找，忽略直流分量 (索引 0)
  for (uint32_t i = 1; i < FFT_N / 2; i++)
  {
    if (fft_magnitudes[i] > max_magnitude)
    {
      max_magnitude = fft_magnitudes[i];
      max_index = i;
    }
  }
  // 计算峰值频率
  // float sampling_frequency = 48000.0f; // 假设的 ADC 采样频率 (Hz)
  float fundamental_frequency = (float)max_index * SAMPLING_FREQ / FFT_N;
  // // fundamental_frequency 即为信号的主频率

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    HAL_Delay(10);
    /* USER CODE END WHILE */

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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
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
