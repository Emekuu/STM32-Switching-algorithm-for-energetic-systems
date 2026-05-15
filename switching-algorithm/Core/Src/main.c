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

#include <stdio.h>
#include <string.h>

#include "com_int.h"
#include "d_decision.h"
#include "link_types.h"
#include "drv_rs485.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

#define WINDOW_SIZE 100U

typedef struct
{
    float    rtt_ms;
    float    peak_rtt_ms;
    float    packet_loss_pct;
    uint8_t  history[WINDOW_SIZE];
    uint16_t head;
    uint32_t total_sent;
    uint32_t total_received;
    uint32_t bytes_in_last_sec;
    uint32_t throughput_kbps;
    uint32_t last_calc_tick;
    uint32_t mem_usage_kb;
} NetworkMetrics_t;

typedef struct __attribute__((packed))
{
    uint32_t msg_id;
    float    rtt;
    float    loss;
    uint32_t throughput;
    uint32_t uptime_sec;
} TelemetryPacket_t;

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
UART_HandleTypeDef huart3;

osThreadId defaultTaskHandle;
osThreadId eth_taskHandle;



static uint8_t g_rs485_ready = 0U;
/* USER CODE BEGIN PV */
NetworkMetrics_t myMetrics = {0};

struct udp_pcb *upcb;
ip_addr_t DestIPaddr;

link_info_t g_links[DECISION_MAX_LINKS];
decision_config_t g_decision_cfg;
decision_result_t g_last_decision;
osThreadId rs_taskHandle;

rs485_if_t g_rs485;
static link_type_t g_active_link = LINK_ETHERNET;
static uint32_t g_last_switch_tick = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_ETH_Init(void);
void StartDefaultTask(void const * argument);
void eth_taskENTRY(void const * argument);
static void App_UpdateBtStub(uint8_t eth_ok);
static void App_UpdateRsStub(uint8_t eth_ok);

/* USER CODE BEGIN PFP */
static void UDP_Client_Connect(void);

static ComStatus_t send_telemetry_to_python(ComInterface_t *comm);
static void Update_Network_Telemetry(NetworkMetrics_t *m,
                                     uint32_t current_rtt,
                                     uint8_t success,
                                     uint16_t data_len);

static void App_InitLinks(void);
static void App_InitDecisionConfig(void);
static link_info_t *App_GetLink(link_type_t type);
static void App_RunDecision(void);
static void App_UpdateEthDown(void);
static void App_UpdateEthReply(uint32_t rtt_ms);
static void App_UpdateEthTimeout(void);


void rs_taskENTRY(void const * argument);

static void App_InitRs485Placeholder(void);
static void App_UpdateRsPath(uint8_t eth_ok);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
    HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  MPU_Config();
  HAL_Init();
  SystemClock_Config();

  /* USER CODE BEGIN Init */
  __HAL_RCC_ETH_CLK_ENABLE();
  __HAL_RCC_ETHMAC_FORCE_RESET();
  HAL_Delay(100);
  __HAL_RCC_ETHMAC_RELEASE_RESET();
  HAL_Delay(100);
  /* USER CODE END Init */

  MX_GPIO_Init();
  MX_USART3_UART_Init();
  MX_ETH_Init();

  /* USER CODE BEGIN 2 */

  App_InitLinks();
  App_InitDecisionConfig();
  App_InitRs485Placeholder();

  printf("Halo, zijem!\r\n");
  printf("[APP] Decision system initialized\r\n");

  /* USER CODE END 2 */

  /* Create the thread(s) */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 256);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  osThreadDef(eth_task, eth_taskENTRY, osPriorityNormal, 0, 512);
  eth_taskHandle = osThreadCreate(osThread(eth_task), NULL);

  osThreadDef(rs_task, rs_taskENTRY, osPriorityBelowNormal, 0, 256);
  rs_taskHandle = osThreadCreate(osThread(rs_task), NULL);

  osKernelStart();

  while (1)
  {
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

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

  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK
                              | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1
                              | RCC_CLOCKTYPE_PCLK2;
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
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{
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
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14 | GPIO_PIN_7, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
static void App_InitRs485Placeholder(void)
{
    (void)g_rs485;
    g_rs485_ready = 0U;
    printf("[RS485] Placeholder mode active\r\n");
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

static void UDP_Client_Connect(void)
{
    upcb = udp_new();
    if (upcb != NULL)
    {
        IP4_ADDR(&DestIPaddr, 192, 168, 56, 1);
        udp_connect(upcb, &DestIPaddr, 12345);
        printf("[UDP] Socket ready\r\n");
    }
    else
    {
        printf("[UDP] Socket create failed\r\n");
    }
}

static ComStatus_t send_telemetry_to_python(ComInterface_t *comm)
{
    TelemetryPacket_t pkt;

    pkt.msg_id = myMetrics.total_sent;
    pkt.rtt = myMetrics.rtt_ms;
    pkt.loss = myMetrics.packet_loss_pct;
    pkt.throughput = myMetrics.throughput_kbps;
    pkt.uptime_sec = osKernelSysTick() / 1000U;

    return comm->Send((uint8_t *)&pkt, sizeof(pkt));
}

static void Update_Network_Telemetry(NetworkMetrics_t *m,
                                     uint32_t current_rtt,
                                     uint8_t success,
                                     uint16_t data_len)
{
    m->history[m->head] = success;
    m->head = (m->head + 1U) % WINDOW_SIZE;
    m->total_sent++;

    if (success)
    {
        m->total_received++;
        m->rtt_ms = (float)current_rtt;
        m->bytes_in_last_sec += data_len;

        if ((float)current_rtt > m->peak_rtt_ms)
        {
            m->peak_rtt_ms = (float)current_rtt;
        }
    }

    uint32_t now = osKernelSysTick();
    if ((now - m->last_calc_tick) >= 1000U)
    {
        m->throughput_kbps = m->bytes_in_last_sec * 8U;
        m->bytes_in_last_sec = 0U;
        m->last_calc_tick = now;
    }

    uint16_t ok_count = 0U;
    uint16_t samples = (m->total_sent < WINDOW_SIZE) ? (uint16_t)m->total_sent : WINDOW_SIZE;

    for (uint16_t i = 0U; i < samples; i++)
    {
        if (m->history[i])
            ok_count++;
    }

    if (samples > 0U)
    {
        m->packet_loss_pct = (1.0f - ((float)ok_count / (float)samples)) * 100.0f;
    }

#if LWIP_STATS
    m->mem_usage_kb = lwip_stats.mem.used / 1024U;
#endif
}

static void App_InitLinks(void)
{
    memset(g_links, 0, sizeof(g_links));

    g_links[0].type = LINK_BLUETOOTH;
    g_links[0].enabled = true;
    g_links[0].state = LINK_STATE_DOWN;

    g_links[1].type = LINK_ETHERNET;
    g_links[1].enabled = true;
    g_links[1].state = LINK_STATE_DOWN;

    Eth_RegisterLinkInfo(&g_links[1]);

    g_links[2].type = LINK_RS485;
    g_links[2].enabled = true;
    g_links[2].state = LINK_STATE_DOWN;
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

    g_decision_cfg.switch_hysteresis_score = 1000;
    g_decision_cfg.min_switch_interval_ms = 5000U;
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
    uint32_t now = osKernelSysTick();
    decision_result_t candidate = decision_select_link(g_links, DECISION_MAX_LINKS, &g_decision_cfg);

    if (!candidate.valid)
    {
        printf("[DECISION] no valid link\r\n");
        return;
    }

    if (candidate.selected_link != g_active_link)
    {
        int32_t current_score = App_GetCurrentLinkScore();
        int32_t score_diff = candidate.selected_score - current_score;

        if (score_diff < g_decision_cfg.switch_hysteresis_score)
        {
            printf("[DECISION] hold current link (hysteresis)\r\n");
            return;
        }

        if ((now - g_last_switch_tick) < g_decision_cfg.min_switch_interval_ms)
        {
            printf("[DECISION] hold current link (cooldown)\r\n");
            return;
        }

        g_active_link = candidate.selected_link;
        g_last_switch_tick = now;
    }

    g_last_decision = candidate;

    printf("[LINKS] ETH(rtt=%lu loss=%u state=%d) | BT(rtt=%lu loss=%u state=%d) | RS(rtt=%lu loss=%u state=%d)\r\n",
           (unsigned long)g_links[1].metrics.rtt_ms,
           g_links[1].metrics.packet_loss_permille,
           g_links[1].state,
           (unsigned long)g_links[0].metrics.rtt_ms,
           g_links[0].metrics.packet_loss_permille,
           g_links[0].state,
           (unsigned long)g_links[2].metrics.rtt_ms,
           g_links[2].metrics.packet_loss_permille,
           g_links[2].state);

    printf("[DECISION] active=%d score=%ld\r\n",
           (int)g_active_link,
           (long)g_last_decision.selected_score);
}

static void App_UpdateEthDown(void)
{
    link_info_t *eth_link = App_GetLink(LINK_ETHERNET);

    if (eth_link == NULL)
        return;

    eth_link->enabled = true;
    eth_link->state = LINK_STATE_DOWN;
    eth_link->metrics.measurement_valid = false;
    eth_link->metrics.rtt_ms = 0U;
    eth_link->metrics.jitter_ms = 0U;
    eth_link->metrics.packet_loss_permille = 1000U;
    eth_link->metrics.signal_dbm = 0;
    eth_link->metrics.security_ok = false;
}

static void App_UpdateEthReply(uint32_t rtt_ms)
{
    link_info_t *eth_link = App_GetLink(LINK_ETHERNET);

    if (eth_link == NULL)
        return;

    eth_link->enabled = true;
    eth_link->state = LINK_STATE_UP;
    eth_link->metrics.measurement_valid = true;
    eth_link->metrics.rtt_ms = rtt_ms;
    eth_link->metrics.jitter_ms = 0U;
    eth_link->metrics.packet_loss_permille = 0U;
    eth_link->metrics.signal_dbm = 0;
    eth_link->metrics.security_ok = true;
}

static void App_UpdateEthTimeout(void)
{
    link_info_t *eth_link = App_GetLink(LINK_ETHERNET);

    if (eth_link == NULL)
        return;

    eth_link->enabled = true;
    eth_link->state = LINK_STATE_DEGRADED;
    eth_link->metrics.measurement_valid = true;
    eth_link->metrics.rtt_ms = 1000U;
    eth_link->metrics.jitter_ms = 300U;
    eth_link->metrics.packet_loss_permille = 1000U;
    eth_link->metrics.signal_dbm = 0;
    eth_link->metrics.security_ok = true;
}

void StartDefaultTask(void const * argument)
{
    printf("Startujem LWIP...\r\n");
    MX_LWIP_Init();
    UDP_Client_Connect();
    printf("LWIP OK!\r\n");

    for (;;)
    {
        osDelay(1);
    }
}

void eth_taskENTRY(void const * argument)
{
    uint8_t rx_buf[64];
    uint16_t rx_len;
    ComInterface_t *comm = &EthernetInterface;

    (void)argument;

    osDelay(5000);

    for (;;)
    {
        if (!comm->IsAlive())
        {
            printf("[ETH] Link DOWN...\r\n");
            App_UpdateEthDown();
            App_UpdateRsStub(0U);
            App_UpdateBtStub(0U);
            App_RunDecision();

            printf("[LINKS] ETH=DOWN BT=UP Active=%d\r\n", (int)g_active_link);

            osDelay(1000);
            continue;
        }

        comm->Init();

        printf("\n[ETH] PING #%lu...", (unsigned long)(myMetrics.total_sent + 1U));
        fflush(stdout);

        uint32_t start_time = osKernelSysTick();

        if (send_telemetry_to_python(comm) == COM_OK)
        {
            uint32_t wait_start = osKernelSysTick();
            uint8_t got_reply = 0U;

            while ((osKernelSysTick() - wait_start) < 1000U)
            {
                if (comm->Receive(rx_buf, sizeof(rx_buf), &rx_len) == COM_OK)
                {
                    uint32_t rtt = osKernelSysTick() - start_time;

                    Update_Network_Telemetry(&myMetrics, rtt, 1U, rx_len);
                    App_UpdateEthReply(rtt);
                    App_UpdateBtStub(1U);
                    App_UpdateRsPath(1U);
                    App_RunDecision();

                    printf(" OK! RTT: %lums | Active: %d",
                           (unsigned long)rtt,
                           (int)g_active_link);

                    got_reply = 1U;
                    break;
                }

                osDelay(10);
            }

            if (!got_reply)
            {
                Update_Network_Telemetry(&myMetrics, 0U, 0U, 0U);
                App_UpdateEthTimeout();
                App_UpdateRsPath(1U);
                App_UpdateBtStub(0U);
                App_RunDecision();

                printf(" TIMEOUT! Active: %d", (int)g_active_link);
            }
        }
        else
        {
            printf(" Chyba odosielania!");
            App_UpdateEthTimeout();
            App_RunDecision();
        }

        fflush(stdout);
        osDelay(20);
    }
}



static void App_UpdateBtStub(uint8_t eth_ok)
{
    link_info_t *bt_link = App_GetLink(LINK_BLUETOOTH);

    if (bt_link == NULL)
        return;

    bt_link->enabled = true;
    bt_link->metrics.measurement_valid = true;
    bt_link->metrics.security_ok = true;

    if (eth_ok)
    {
        bt_link->state = LINK_STATE_UP;
        bt_link->metrics.rtt_ms = 80U;
        bt_link->metrics.jitter_ms = 15U;
        bt_link->metrics.packet_loss_permille = 50U;
        bt_link->metrics.signal_dbm = -65;
    }
    else
    {
        bt_link->state = LINK_STATE_UP;
        bt_link->metrics.rtt_ms = 90U;
        bt_link->metrics.jitter_ms = 20U;
        bt_link->metrics.packet_loss_permille = 80U;
        bt_link->metrics.signal_dbm = -65;
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

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  HAL_MPU_Disable();

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
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
