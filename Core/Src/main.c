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
// #include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "fft.h"              // 包含自定义的 FFT 头文件
#include <math.h>             // 包含数学库
#include <stdio.h>            // 添加: 包含标准输入输出库 (用于 sprintf)
#include <string.h>           // 添加: 包含字符串库 (用于 strlen)
#include "all_DLC_includes.h" // 包含STM32USB库
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// 定义 USB CDC 接收缓冲区大小 (需要与 usbd_cdc_if.c 中的 APP_RX_DATA_SIZE 匹配，通常是 64 或 2048)
// 如果不确定，可以先定义为较大的值，例如 256
#define USB_RX_BUFFER_SIZE 256

#define ADC_BUFFER_SIZE FFT_N // 假设 ADC 采样点数与 FFT 点数相同
// --- 采样频率 (固定) ---
#define SAMPLING_FREQ 48000.0f // 假设的采样频率 (Hz)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

// --- 当前信号参数 (可由 USB 更新) ---
// 使用 volatile 确保编译器不会过度优化对这些变量的访问
volatile float current_signal_freq = 1000.0f;   // 当前正弦波频率 (Hz)
volatile float current_signal_amplitude = 1.0f; // 当前正弦波幅度
volatile float current_signal_offset = 0.0f;    // 当前正弦波直流偏移
volatile uint8_t new_parameters_received = 1;   // 标志位，指示是否收到新参数 (初始设为1，以便启动时计算一次)

// --- USB 缓冲区 ---
char usb_tx_buffer[128];                   // 用于格式化输出的缓冲区
uint8_t usb_rx_buffer[USB_RX_BUFFER_SIZE]; // USB CDC 接收缓冲区

// --- FFT 相关缓冲区 ---
float adc_samples[ADC_BUFFER_SIZE]; // 存储生成的采样数据的数组
complex_t fft_input_output[FFT_N];  // FFT 输入/输出缓冲区 (复数形式)
float fft_magnitudes[FFT_N / 2];    // 存储 FFT 幅度结果的数组

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
// 函数声明：执行 FFT 计算并发送结果
void perform_fft_and_send(void);
// 函数声明：处理接收到的 USB 数据 (将在 CDC_Receive_FS 中调用)
void process_usb_data(uint8_t *Buf, uint32_t Len);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief 更新信号参数的函数 (供 usbd_cdc_if 调用)
 * @param freq: 新频率
 * @param amp: 新幅度
 * @param offset: 新偏移
 */
void Update_Signal_Parameters(float freq, float amp, float offset)
{
  // 添加参数验证和调试输出
  sprintf(usb_tx_buffer, "DEBUG-U: Params: F=%.2f, A=%.2f, O=%.2f\r\n", freq, amp, offset);
  CDC_Transmit_FS((uint8_t *)usb_tx_buffer, strlen(usb_tx_buffer));
  
  // 增加范围限制，防止无效值
  if (freq >= 1.0f && freq <= (SAMPLING_FREQ / 2.0f))
  {
    current_signal_freq = freq;
  }
  if (amp >= 0.0f)
  {
    current_signal_amplitude = amp;
  }
  current_signal_offset = offset;
}

/**
 * @brief 设置标志位以触发 FFT 重新计算 (供 usbd_cdc_if 调用)
 */
void Trigger_FFT_Recalculation(void)
{
  // 添加调试输出，查看标志状态
  sprintf(usb_tx_buffer, "DEBUG-T: Flag before=%d\r\n", (int)new_parameters_received);
  CDC_Transmit_FS((uint8_t *)usb_tx_buffer, strlen(usb_tx_buffer));
  
  // 设置标志位
  new_parameters_received = 1;
  
  // 设置后再次检查并输出，确认设置成功
  sprintf(usb_tx_buffer, "DEBUG-T: Flag after=%d\r\n", (int)new_parameters_received);
  CDC_Transmit_FS((uint8_t *)usb_tx_buffer, strlen(usb_tx_buffer));
  
  // 添加一个内存屏障操作，确保标志位值立即可见（防止编译器优化）
  __DSB(); // 数据同步屏障
}

/**
 * @brief 生成正弦波，执行 FFT 并通过 USB 发送结果
 */
void perform_fft_and_send(void)
{
  // ... perform_fft_and_send 函数的实现保持不变 ...
  // --- 1. 使用当前参数生成模拟正弦波信号 ---
  // 读取 volatile 变量到局部变量，避免在循环中重复读取
  float freq = current_signal_freq;
  float amp = current_signal_amplitude;
  float offset = current_signal_offset;

  for (int i = 0; i < ADC_BUFFER_SIZE; i++)
  {
    adc_samples[i] = amp * sinf(2.0f * M_PI * freq * (float)i / SAMPLING_FREQ) + offset;
  }

  // --- 2. 准备 FFT 输入缓冲区 ---
  for (int i = 0; i < FFT_N; i++)
  {
    if (i < ADC_BUFFER_SIZE)
    {
      fft_input_output[i].real = adc_samples[i];
    }
    else
    {
      fft_input_output[i].real = 0.0f; // 零填充
    }
    fft_input_output[i].imag = 0.0f; // 虚部初始化为 0
  }

  // --- 3. 执行 FFT 计算 ---
  fft_radix2(fft_input_output, FFT_N);

  // --- 4. 计算 FFT 结果的幅度 ---
  fft_calculate_magnitudes(fft_input_output, fft_magnitudes, FFT_N);

  // --- 5. 通过 USB VCP 发送 FFT 幅度结果 ---
  sprintf(usb_tx_buffer, "--- FFT Magnitudes (F:%.1fHz A:%.2f O:%.2f) ---\r\n", freq, amp, offset);
  CDC_Transmit_FS((uint8_t *)usb_tx_buffer, strlen(usb_tx_buffer));
  HAL_Delay(10); // 短暂延时

  for (uint32_t i = 0; i < FFT_N / 2; i++)
  {
    int len = sprintf(usb_tx_buffer, "FFT[%lu]: %.4f\r\n", i, fft_magnitudes[i]);
    uint8_t result = CDC_Transmit_FS((uint8_t *)usb_tx_buffer, len);
    if (result != USBD_OK)
    {
      HAL_Delay(1); // 发送失败时短暂延时
                    // 可以添加重试逻辑或错误计数
    }
    HAL_Delay(2); // 每行之间短暂延时，防止发送过快
  }

  sprintf(usb_tx_buffer, "--- FFT Transmission Complete ---\r\n");
  CDC_Transmit_FS((uint8_t *)usb_tx_buffer, strlen(usb_tx_buffer));
  HAL_Delay(10); // 发送完成后的短暂延时

  // --- 6. (可选) 计算并发送峰值频率 ---
  float max_magnitude = 0;
  uint32_t max_index = 0;
  for (uint32_t i = 1; i < FFT_N / 2; i++) // 忽略直流分量
  {
    if (fft_magnitudes[i] > max_magnitude)
    {
      max_magnitude = fft_magnitudes[i];
      max_index = i;
    }
  }
  float fundamental_frequency = (float)max_index * SAMPLING_FREQ / FFT_N;
  sprintf(usb_tx_buffer, "Peak Frequency Index: %lu (%.2f Hz)\r\n", max_index, fundamental_frequency);
  CDC_Transmit_FS((uint8_t *)usb_tx_buffer, strlen(usb_tx_buffer));
  HAL_Delay(10);
}
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
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(3000); // 等我插上USB

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // 每隔几秒发送一次当前参数状态，以便诊断
    static uint32_t lastDebugTime = 0;
    uint32_t currentTime = HAL_GetTick();
    
    if (currentTime - lastDebugTime > 3000) // 每3秒发送一次
    {
      lastDebugTime = currentTime;
      sprintf(usb_tx_buffer, "DEBUG: Flag=%d, F=%.1f, A=%.2f, O=%.2f\r\n", 
              (int)new_parameters_received,
              current_signal_freq,
              current_signal_amplitude,
              current_signal_offset);
      CDC_Transmit_FS((uint8_t *)usb_tx_buffer, strlen(usb_tx_buffer));
    }
    
    if (new_parameters_received)
    {
      new_parameters_received = 0;                // 清除标志位
      perform_fft_and_send();                     // 执行 FFT 计算和发送
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin); // 切换 LED 状态，指示处理完成
    }

    // 主循环可以执行其他低优先级任务
    HAL_Delay(10); // 短暂延时，降低 CPU 占用率，但会影响响应速度
  }
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
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
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
