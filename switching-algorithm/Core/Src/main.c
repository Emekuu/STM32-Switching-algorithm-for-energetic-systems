/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "lwip.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/stats.h"
#include "lwip/udp.h"

#include <stdio.h>
#include <string.h>
#include "drv_bt.h"
#include "com_int.h"
#include "d_decision.h"
#include "link_types.h"
#include "drv_rs485.h"
#include "eth_parser.h"
#include "eth_metrics.h"
#include "eth_rx_types.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
extern struct netif gnetif;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */




/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
extern ETH_DMADescTypeDef DMARxDscrTab[ETH_RXBUFNB];
extern ETH_DMADescTypeDef DMATxDscrTab[ETH_TXBUFNB];
extern ETH_HandleTypeDef heth;
extern ComInterface_t EthernetInterface;

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef hdma_i2c1_rx;
DMA_HandleTypeDef hdma_i2c1_tx;

UART_HandleTypeDef huart3;

osThreadId defaultTaskHandle;
osThreadId eth_taskHandle;
/* USER CODE BEGIN PV */
static uint8_t g_rs485_ready = 0U;
static struct udp_pcb *g_rx_pcb = NULL;

static rx_metrics_t g_rx_direct;
static rx_metrics_t g_rx_plc;
static rx_metrics_t g_rx_bt;
static rx_metrics_t g_rx_rs485;
link_info_t g_links[DECISION_MAX_LINKS];
decision_config_t g_decision_cfg;
decision_result_t g_last_decision;
osThreadId rs_taskHandle;

rs485_if_t g_rs485;
static link_type_t g_active_link = LINK_ETH_DIRECT;
static uint32_t g_last_switch_tick = 0U;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_I2C1_Init(void);
void StartDefaultTask(void const * argument);
void eth_taskENTRY(void const * argument);

/* USER CODE BEGIN PFP */
static void App_UpdateRsStub(uint8_t eth_ok);
static const char *App_LinkTypeToStr(link_type_t type);
static void App_InitLinks(void);
static void App_InitDecisionConfig(void);
static link_info_t *App_GetLink(link_type_t type);
static void App_RunDecision(void);


static void App_UpdateBtLink(void);
static void App_UpdateRsStub(uint8_t eth_ok);
static void MX_ETH_Init(void);
void rs_taskENTRY(void const * argument);

static void App_InitRs485Placeholder(void);
static void App_UpdateRsPath(uint8_t eth_ok);


static void Eth_RxInit(void);
static void Eth_UdpRecvCallback(void *arg,
                                struct udp_pcb *pcb,
                                struct pbuf *p,
                                const ip_addr_t *addr,
                                u16_t port);

static rx_metrics_t *Eth_GetMetricsByPath(uint8_t path_id);
static link_type_t Eth_GetLinkTypeByPath(uint8_t path_id);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
static void App_PrintNetifInfo(void)
{
    const ip4_addr_t *ip = netif_ip4_addr(&gnetif);
    const ip4_addr_t *nm = netif_ip4_netmask(&gnetif);
    const ip4_addr_t *gw = netif_ip4_gw(&gnetif);

    char ip_str[16];
    char nm_str[16];
    char gw_str[16];

    ip4addr_ntoa_r(ip, ip_str, sizeof(ip_str));
    ip4addr_ntoa_r(nm, nm_str, sizeof(nm_str));
    ip4addr_ntoa_r(gw, gw_str, sizeof(gw_str));

    printf("[LWIP_INIT] IP=%s MASK=%s GW=%s\r\n", ip_str, nm_str, gw_str);
}
PUTCHAR_PROTOTYPE
{
    HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}

static rx_metrics_t *Eth_GetMetricsByPath(uint8_t path_id)
{
    switch ((path_id_t)path_id)
    {
        case PATH_ETH_DIRECT: return &g_rx_direct;
        case PATH_ETH_PLC:    return &g_rx_plc;
        case PATH_BT:         return &g_rx_bt;
        case PATH_RS485:      return &g_rx_rs485;
        default:              return NULL;
    }
}

static link_type_t Eth_GetLinkTypeByPath(uint8_t path_id)
{
    switch ((path_id_t)path_id)
    {
        case PATH_ETH_DIRECT: return LINK_ETH_DIRECT;
        case PATH_ETH_PLC:    return LINK_ETH_PLC;
        case PATH_BT:         return LINK_BLUETOOTH;
        case PATH_RS485:      return LINK_RS485;
        default:              return LINK_NONE;
    }
}


static void Eth_RxInit(void)
{
    err_t err;

    Metrics_Reset(&g_rx_direct);
    Metrics_Reset(&g_rx_plc);
    Metrics_Reset(&g_rx_bt);
    Metrics_Reset(&g_rx_rs485);

    g_rx_pcb = udp_new();
    if (g_rx_pcb == NULL)
    {
        printf("[RX] udp_new failed\r\n");
        return;
    }

    err = udp_bind(g_rx_pcb, IP_ADDR_ANY, 5005);
    if (err != ERR_OK)
    {
        printf("[RX] udp_bind failed: %d\r\n", (int)err);
        udp_remove(g_rx_pcb);
        g_rx_pcb = NULL;
        return;
    }

    udp_recv(g_rx_pcb, Eth_UdpRecvCallback, NULL);
    printf("[RX] UDP receiver ready on port 5005\r\n");
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

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  __HAL_RCC_ETH_CLK_ENABLE();
  __HAL_RCC_ETHMAC_FORCE_RESET();
  HAL_Delay(100);
  __HAL_RCC_ETHMAC_RELEASE_RESET();
  HAL_Delay(100);
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART3_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  MX_ETH_Init();
  App_InitLinks();
  App_InitDecisionConfig();
  App_InitRs485Placeholder();

  printf("Halo, zijem!\r\n");
  printf("[APP] Decision system initialized\r\n");

  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityHigh, 0, 256);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of eth_task */
  osThreadDef(eth_task, eth_taskENTRY, osPriorityBelowNormal, 0, 1024);
  eth_taskHandle = osThreadCreate(osThread(eth_task), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 216;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
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
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pins : PB14 PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

static void App_UpdateBtLink(void)
{
    bt_i2c_metrics_t btm;
    link_info_t *bt_link = App_GetLink(LINK_BLUETOOTH);

    if (bt_link == NULL)
        return;

    bt_link->enabled = true;
    bt_link->metrics.security_ok = true;

    if (BT_I2C_ReadMetrics(&btm) != HAL_OK)
    {
        printf("[BT] I2C read FAILED\r\n");
        bt_link->state = LINK_STATE_DOWN;
        bt_link->metrics.measurement_valid = false;
        bt_link->metrics.packet_loss_permille = 1000U;
        bt_link->metrics.jitter_ms = 0U;
        bt_link->metrics.rtt_ms = 1000U;
        bt_link->metrics.signal_dbm = -128;
        return;
    }

    printf("[BT] avail=%u rssi=%d loss=%u rtt=%u jitter=%u\r\n",
           (unsigned)btm.available,
           (int)btm.rssi,
           (unsigned)btm.pkt_error_rate,
           (unsigned)btm.rtt_ms,
           (unsigned)btm.jitter_ms);

    bt_link->metrics.measurement_valid = (btm.available != 0U);
    bt_link->metrics.signal_dbm = btm.rssi;

    if (btm.pkt_error_rate > 1000U)
        btm.pkt_error_rate = 1000U;

    bt_link->metrics.packet_loss_permille = btm.pkt_error_rate;

    if (btm.available)
    {
        bt_link->state = LINK_STATE_UP;
        bt_link->metrics.rtt_ms    = btm.rtt_ms;
        bt_link->metrics.jitter_ms = btm.jitter_ms;
    }
    else
    {
        bt_link->state = LINK_STATE_DOWN;
        bt_link->metrics.rtt_ms = 1000U;
        bt_link->metrics.jitter_ms = 0U;
    }
}
static void App_UpdateEthLinkHealth(void)
{
    uint32_t now = HAL_GetTick();
    const uint32_t timeout_ms = 2000U;

    link_info_t *eth_direct = App_GetLink(LINK_ETH_DIRECT);
    link_info_t *eth_plc = App_GetLink(LINK_ETH_PLC);

    Metrics_Age(&g_rx_direct, now, timeout_ms);
    Metrics_Age(&g_rx_plc, now, timeout_ms);

    Metrics_ApplyToLink(eth_direct, &g_rx_direct);
    Metrics_ApplyToLink(eth_plc, &g_rx_plc);

    if (Metrics_IsTimedOut(&g_rx_direct, now, timeout_ms) && (eth_direct != NULL))
    {
        eth_direct->state = LINK_STATE_DOWN;
        eth_direct->metrics.measurement_valid = false;
        eth_direct->metrics.packet_loss_permille = 1000U;
        eth_direct->metrics.jitter_ms = 0U;
        eth_direct->metrics.rtt_ms = 1000U;
    }

    if (Metrics_IsTimedOut(&g_rx_plc, now, timeout_ms) && (eth_plc != NULL))
    {
        eth_plc->state = LINK_STATE_DOWN;
        eth_plc->metrics.measurement_valid = false;
        eth_plc->metrics.packet_loss_permille = 1000U;
        eth_plc->metrics.jitter_ms = 0U;
        eth_plc->metrics.rtt_ms = 1000U;
    }
}
static void Eth_UdpRecvCallback(void *arg,
                                struct udp_pcb *pcb,
                                struct pbuf *p,
                                const ip_addr_t *addr,
                                u16_t port)
{
    (void)arg;
    (void)pcb;
    (void)addr;
    (void)port;

    if (p == NULL)
        return;

    printf("[RX_RAW] got UDP len=%u\r\n", (unsigned int)p->tot_len);

    if ((p->payload != NULL) && (p->len > 0U))
    {
        generic_frame_info_t frame;
        uint32_t now_ms = HAL_GetTick();

        if (Generic_ParseFrame((const uint8_t *)p->payload, p->len, &frame))
        {
            rx_metrics_t *metrics = Eth_GetMetricsByPath(frame.path_id);
            link_type_t link_type = Eth_GetLinkTypeByPath(frame.path_id);
            link_info_t *link = App_GetLink(link_type);

            if ((metrics != NULL) && (link != NULL))
            {
                printf("[DBG] seq=%lu tx=%lu now=%lu diff=%ld\r\n",
                       (unsigned long)frame.seq,
                       (unsigned long)frame.tx_ts_ms,
                       (unsigned long)now_ms,
                       (long)((int32_t)(now_ms - frame.tx_ts_ms)));

                Metrics_OnFrameReceived(metrics, &frame, now_ms);
                Metrics_ApplyToLink(link, metrics);

                printf("[RX] path=%u seq=%lu len=%lu loss=%lu permille jitter=%lu ms tp=%lu bps\r\n",
                       (unsigned int)frame.path_id,
                       (unsigned long)frame.seq,
                       (unsigned long)frame.payload_len,
                       (unsigned long)link->metrics.packet_loss_permille,
                       (unsigned long)Metrics_GetJitterMs(metrics),
                       (unsigned long)Metrics_GetThroughputBps(metrics));

                /* <<< TOTO JE PRE LOGGER >>> */
                printf("CSV,%lu,%u,%lu,%lu,%lu,%lu\r\n",
                       (unsigned long)now_ms,
                       (unsigned int)frame.path_id,
                       (unsigned long)link->metrics.packet_loss_permille,
                       (unsigned long)Metrics_GetJitterMs(metrics),
                       (unsigned long)Metrics_GetThroughputBps(metrics),
                       (unsigned long)frame.seq);
                printf("[RX_SRC] src=%s:%u path=%u seq=%lu\r\n",
                       ipaddr_ntoa(addr),
                       (unsigned)port,
                       (unsigned)frame.path_id,
                       (unsigned long)frame.seq);

                App_RunDecision();
            }
            else
            {
                printf("[RX] unknown path_id=%u\r\n", (unsigned int)frame.path_id);
            }
        }
        else
        {
            printf("[RX] parse failed, len=%u\r\n", (unsigned int)p->len);
        }
    }

    pbuf_free(p);
}
static void App_InitRs485Placeholder(void)
{
    (void)g_rs485;
    g_rs485_ready = 0U;
    printf("[RS485] Placeholder mode active\r\n");
}

static void App_MaybeHardResetEthMetrics(uint32_t now_ms)
{
    const uint32_t hard_reset_timeout_ms = 10000U; // napr. 10 s bez framov

    if (Metrics_IsTimedOut(&g_rx_direct, now_ms, hard_reset_timeout_ms)) {
        Metrics_Reset(&g_rx_direct);
    }
    if (Metrics_IsTimedOut(&g_rx_plc, now_ms, hard_reset_timeout_ms)) {
        Metrics_Reset(&g_rx_plc);
    }
}

static void App_UpdateRsPath(uint8_t eth_ok)
{
    if (g_rs485_ready)
    {
        /* Neskôr sem pôjde reálny RS485 update */
        App_UpdateRsStub(eth_ok);
    }
    else
    {
        App_UpdateRsStub(eth_ok);
    }
}

void rs_taskENTRY(void const * argument)
{
    (void)argument;

    printf("[RS485] Task started\r\n");

    for (;;)
    {
        if (g_rs485_ready)
        {
            /*request/reply polling through RS485 */
        }

        osDelay(200);
    }
}

static void MX_ETH_Init(void)
{
    heth.Instance = ETH;
    heth.Init.MACAddr = (uint8_t *)"0080E1";
    heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
    heth.Init.TxDesc = DMATxDscrTab;
    heth.Init.RxDesc = DMARxDscrTab;
    heth.Init.RxBuffLen = 1524;

    if (HAL_ETH_Init(&heth) != HAL_OK)
    {
        printf("ETH Hardware Error!\r\n");
    }
}



static const char *App_LinkTypeToStr(link_type_t type)
{
    switch (type)
    {
        case LINK_BLUETOOTH:  return "BT";
        case LINK_ETH_DIRECT: return "ETH_DIRECT";
        case LINK_ETH_PLC:    return "ETH_PLC";
        case LINK_RS485:      return "RS485";
        default:              return "UNKNOWN";
    }
}


static void App_InitLinks(void)
{
    memset(g_links, 0, sizeof(g_links));

    /* Bluetooth link */
    g_links[0].type = LINK_BLUETOOTH;
    g_links[0].enabled = true;
    g_links[0].state = LINK_STATE_DOWN;

    /* Ethernet DIRECT link */
    g_links[1].type = LINK_ETH_DIRECT;
    g_links[1].enabled = true;
    g_links[1].state = LINK_STATE_DOWN;

    /* Ethernet PLC link */
    g_links[2].type = LINK_ETH_PLC;
    g_links[2].enabled = true;
    g_links[2].state = LINK_STATE_DOWN;

    /* RS485 link */
    g_links[3].type = LINK_RS485;
    g_links[3].enabled = true;
    g_links[3].state = LINK_STATE_DOWN;

}

static void App_InitDecisionConfig(void)
{
    g_decision_cfg.algorithm = DECISION_ALG_WEIGHTED_SCORE;
    g_decision_cfg.security_policy = SECURITY_PREFERRED;

    g_decision_cfg.max_rtt_ms = 1000U;
    g_decision_cfg.max_jitter_ms = 300U;
    g_decision_cfg.max_loss_permille = 1000U;

    g_decision_cfg.weight_rtt = 5;
    g_decision_cfg.weight_jitter = 3;
    g_decision_cfg.weight_loss = 4;
    g_decision_cfg.weight_signal = 1;
    g_decision_cfg.weight_security = 2;

    g_decision_cfg.threshold_rtt_ms = 300U;
    g_decision_cfg.threshold_jitter_ms = 100U;
    g_decision_cfg.threshold_loss_permille = 300U;

    g_decision_cfg.recovery_rtt_ms = 150U;
    g_decision_cfg.recovery_jitter_ms = 50U;
    g_decision_cfg.recovery_loss_permille = 100U;

    g_decision_cfg.switch_hysteresis_score = 100;
    g_decision_cfg.min_switch_interval_ms = 1000U;
}
static link_info_t *App_GetLink(link_type_t type)
{
    for (uint32_t i = 0U; i < DECISION_MAX_LINKS; i++)
    {
        if (g_links[i].type == type)
            return &g_links[i];
    }

    return NULL;
}

static int32_t App_GetCurrentLinkScore(void)
{
    decision_result_t current_result;
    link_info_t *current_link = App_GetLink(g_active_link);

    if (current_link == NULL)
        return (-2147483647 - 1);

    current_result = decision_select_link(current_link, 1U, &g_decision_cfg);

    if (!current_result.valid)
        return (-2147483647 - 1);

    return current_result.selected_score;
}


static void App_RunDecision(void)
{
    uint32_t now = HAL_GetTick();
    decision_result_t candidate = decision_select_link(g_links, DECISION_MAX_LINKS, &g_decision_cfg);

    if (!candidate.valid)
    {
        printf("[DECISION] no valid link\r\n");
        return;
    }

    if (candidate.selected_link != g_active_link)
    {
        int32_t current_score = App_GetCurrentLinkScore();

        if (current_score == (-2147483647 - 1))
        {
            printf("[DECISION] current link invalid, switching to %s immediately\r\n",
                   App_LinkTypeToStr(candidate.selected_link));
        }
        else
        {
            int32_t score_diff = candidate.selected_score - current_score;

            if (score_diff < g_decision_cfg.switch_hysteresis_score)
            {
            	printf("[DECISION] hold current link (hysteresis), keep=%s\r\n",
            	       App_LinkTypeToStr(g_active_link));
                return;
            }

            if ((now - g_last_switch_tick) < g_decision_cfg.min_switch_interval_ms)
            {
                printf("[DECISION] hold current link (cooldown)\r\n");
                return;
            }
        }

        g_active_link = candidate.selected_link;
        g_last_switch_tick = now;
    }

    g_last_decision = candidate;

    printf("[LINKS] BT(rtt=%lu loss=%lu state=%d) | ETH_DIR(rtt=%lu loss=%lu state=%d) | "
           "ETH_PLC(rtt=%lu loss=%lu state=%d) | RS(rtt=%lu loss=%lu state=%d)\r\n",
           (unsigned long)g_links[0].metrics.rtt_ms,
           (unsigned long)g_links[0].metrics.packet_loss_permille,
           g_links[0].state,
           (unsigned long)g_links[1].metrics.rtt_ms,
           (unsigned long)g_links[1].metrics.packet_loss_permille,
           g_links[1].state,
           (unsigned long)g_links[2].metrics.rtt_ms,
           (unsigned long)g_links[2].metrics.packet_loss_permille,
           g_links[2].state,
           (unsigned long)g_links[3].metrics.rtt_ms,
           (unsigned long)g_links[3].metrics.packet_loss_permille,
           g_links[3].state);

    printf("[DECISION] active=%s score=%ld\r\n",
           App_LinkTypeToStr(g_active_link),
           (long)g_last_decision.selected_score);
}








static inline link_type_t PathIdToLinkType(path_id_t path)
{
    switch (path)
    {
        case PATH_ETH_DIRECT: return LINK_ETH_DIRECT;
        case PATH_ETH_PLC:    return LINK_ETH_PLC;
        case PATH_BT:         return LINK_BLUETOOTH;
        case PATH_RS485:      return LINK_RS485;
        default:              return LINK_NONE;
    }
}
static void App_UpdateRsStub(uint8_t eth_ok)
{
    link_info_t *rs_link = App_GetLink(LINK_RS485);

    if (rs_link == NULL)
        return;

    rs_link->enabled = true;
    rs_link->metrics.measurement_valid = true;
    rs_link->metrics.security_ok = true;

    if (eth_ok)
    {
        rs_link->state = LINK_STATE_UP;
        rs_link->metrics.rtt_ms = 140U;
        rs_link->metrics.jitter_ms = 25U;
        rs_link->metrics.packet_loss_permille = 120U;
        rs_link->metrics.signal_dbm = 0;
    }
    else
    {
        rs_link->state = LINK_STATE_UP;
        rs_link->metrics.rtt_ms = 70U;
        rs_link->metrics.jitter_ms = 10U;
        rs_link->metrics.packet_loss_permille = 20U;
        rs_link->metrics.signal_dbm = 0;
    }
}








/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN 5 */
	  Eth_RxInit();
	  App_PrintNetifInfo();
	  printf("LWIP OK!\r\n");

	  for(;;)
	  {
	    uint8_t eth_ok = 0U;
	    uint32_t now = HAL_GetTick();
	    const uint32_t timeout_ms = 2000U;

	    App_UpdateEthLinkHealth();
	    App_MaybeHardResetEthMetrics(now);
	    if (!Metrics_IsTimedOut(&g_rx_direct, now, timeout_ms) ||
	        !Metrics_IsTimedOut(&g_rx_plc, now, timeout_ms))
	    {
	        eth_ok = 1U;
	    }

	    App_UpdateBtLink();
	    App_UpdateRsPath(eth_ok);

	    App_RunDecision();
	    osDelay(100);
	  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_eth_taskENTRY */
/**
* @brief Function implementing the eth_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_eth_taskENTRY */
void eth_taskENTRY(void const * argument)
{
  /* USER CODE BEGIN eth_taskENTRY */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END eth_taskENTRY */
}

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
