/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

COM_InitTypeDef BspCOMInit;
I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */
uint8_t RX_Buffer[1];
uint8_t TX_Frame[5];   // [MSG_TYPE, RSSI, ERR, STATUS, CS]
uint8_t RX_Frame[5];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t CalcChecksum(uint8_t *buf, uint8_t len)
{
    uint8_t cs = 0;
    for (uint8_t i = 0; i < len; i++)
        cs ^= buf[i];
    return cs;
}

void BuildStatusFrame(void)
{
    int8_t  rssi   = -65;  // zatiaľ natvrdo, neskôr z BLE stacku
    uint8_t err    = 3;    // testovacia hodnota
    uint8_t status = 0x01; // bit0 = link UP

    TX_Frame[0] = 0x01;          // MSG_TYPE = BLE_STATUS
    TX_Frame[1] = (uint8_t)rssi;
    TX_Frame[2] = err;
    TX_Frame[3] = status;
    TX_Frame[4] = CalcChecksum(TX_Frame, 4);
}

typedef struct {
    uint16_t rtt_ms;
    uint16_t loss_permille;
    int16_t rssi;
    uint16_t jitter_ms;
} metrics_t;

metrics_t metrics;
uint8_t  metrics_buf[8];



void metrics_update_buffer(void)
{
    metrics_buf[0] = metrics.rtt_ms >> 8;
    metrics_buf[1] = metrics.rtt_ms & 0xFF;
    metrics_buf[2] = metrics.loss_permille >> 8;
    metrics_buf[3] = metrics.loss_permille & 0xFF;
    metrics_buf[4] = metrics.rssi >> 8;
    metrics_buf[5] = metrics.rssi & 0xFF;
    metrics_buf[6] = metrics.jitter_ms >> 8;
    metrics_buf[7] = metrics.jitter_ms & 0xFF;
}


#define PING_INTERVAL_MS  100
#define WINDOW_MS         2000
#define LOSS_THRESHOLD_PM 300

typedef struct {
    uint16_t id;
    uint32_t t_send_ms;
} ping_entry_t;

#define PING_TABLE_SIZE 16

static ping_entry_t ping_table[PING_TABLE_SIZE];
static uint16_t next_ping_id;
static uint32_t sent_pings, received_pongs, sum_rtt_ms;
static uint32_t now_ms, window_start_ms, last_ping_ms;

uint32_t get_time_ms(void) { return HAL_GetTick(); } // stačí SysTick
void link_send_ble(uint8_t *data, uint16_t len); // už máš v svojom BLE code

void send_ping(void)
{
    now_ms = get_time_ms();
    uint16_t id = next_ping_id++;

    ping_entry_t *e = &ping_table[id % PING_TABLE_SIZE];
    e->id = id;
    e->t_send_ms = now_ms;

    uint8_t frame[3];
    frame[0] = 0x10;           // ping
    frame[1] = id >> 8;
    frame[2] = id & 0xFF;

    link_send_ble(frame, 3);
    sent_pings++;
}

void on_ble_frame_rx(uint8_t *data, uint16_t len)
{
    if (len < 3) return;
    uint8_t  type = data[0];
    uint16_t id   = (data[1] << 8) | data[2];

    if (type == 0x11) { // pong
        now_ms = get_time_ms();
        ping_entry_t *e = &ping_table[id % PING_TABLE_SIZE];
        if (e->id != id) return;

        uint32_t rtt = now_ms - e->t_send_ms;
        sum_rtt_ms += rtt;
        received_pongs++;
        e->id = 0xFFFF;
    }
}

void compute_window_metrics(void)
{
    uint32_t lost = (sent_pings > received_pongs)
                    ? (sent_pings - received_pongs) : 0;

    uint16_t loss_pm = 0;
    if (sent_pings > 0)
        loss_pm = (uint16_t)((lost * 1000U) / sent_pings);

    uint16_t avg_rtt = 0;
    if (received_pongs > 0)
        avg_rtt = (uint16_t)(sum_rtt_ms / received_pongs);

    metrics.rtt_ms        = avg_rtt;
    metrics.loss_permille = loss_pm;
    metrics.rssi          = -65; // zatiaľ natvrdo, neskôr z BLE RSSI API[web:369][web:393]
    metrics.jitter_ms     = 0;               // môžeš dopočítať neskôr z variácie RTT
}

void reset_window(void)
{
    window_start_ms = now_ms;
    sent_pings = received_pongs = sum_rtt_ms = 0;
}

void link_probe_task(void)
{
    now_ms = get_time_ms();

    if ((now_ms - last_ping_ms) >= PING_INTERVAL_MS) {
        send_ping();
        last_ping_ms = now_ms;
    }

    if ((now_ms - window_start_ms) >= WINDOW_MS) {
        compute_window_metrics();
        reset_window();
    }
}





static uint8_t bt_frame[8];

static uint8_t BT_I2C_CalcChecksum(const uint8_t *buf, uint8_t len)
{
    uint8_t cs = 0;
    for (uint8_t i = 0; i < len; i++)
        cs ^= buf[i];
    return cs;
}

static void BT_I2C_BuildFrame(void)
{
    bt_frame[0] = 0xA5;   // header
    bt_frame[1] = (uint8_t)((int8_t)metrics.rssi);
    bt_frame[2] = (uint8_t)(metrics.loss_permille >> 8);
    bt_frame[3] = (uint8_t)(metrics.loss_permille & 0xFF);
    bt_frame[4] = (uint8_t)(metrics.rtt_ms >> 8);
    bt_frame[5] = (uint8_t)(metrics.rtt_ms & 0xFF);
    bt_frame[6] = (uint8_t)(metrics.jitter_ms & 0xFF); // zatiaľ len LSB jitteru
    bt_frame[7] = BT_I2C_CalcChecksum(bt_frame, 7);
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

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */

  HAL_StatusTypeDef ret;

  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_RED);
  /* USER CODE END 2 */

  /* Initialize leds */
  BSP_LED_Init(LED_BLUE);
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_RED);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_SW1, BUTTON_MODE_EXTI);
  BSP_PB_Init(BUTTON_SW2, BUTTON_MODE_EXTI);
  BSP_PB_Init(BUTTON_SW3, BUTTON_MODE_EXTI);

  /* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
  BspCOMInit.BaudRate   = 115200;
  BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  BspCOMInit.StopBits   = COM_STOPBITS_1;
  BspCOMInit.Parity     = COM_PARITY_NONE;
  BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
  if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */


  while (1)
  {
      metrics.rtt_ms = 42;
      metrics.loss_permille = 15;
      metrics.rssi = -65;
      metrics.jitter_ms = 3;

      BT_I2C_BuildFrame();
      HAL_I2C_Slave_Transmit(&hi2c1, bt_frame, sizeof(bt_frame), HAL_MAX_DELAY);
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
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 27;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK4|RCC_CLOCKTYPE_HCLK2
                              |RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK2Divider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.AHBCLK4Divider = RCC_SYSCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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
  hi2c1.Init.Timing = 0x20404768;
  hi2c1.Init.OwnAddress1 = 80;
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
