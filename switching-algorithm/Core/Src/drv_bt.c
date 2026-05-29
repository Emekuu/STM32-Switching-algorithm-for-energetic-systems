#include "drv_bt.h"
#include "main.h"
#include <stdint.h>
#include <stdio.h>

#define BT_I2C_FRAME_SIZE 8
#define BT_I2C_SLAVE_ADDR_7BIT 0x28
#define BT_I2C_DEFAULT_TIMEOUT 100

extern I2C_HandleTypeDef hi2c1;

static uint8_t g_bt_i2c_rx_frame[BT_I2C_FRAME_SIZE];

static uint8_t BT_I2C_CalcChecksum(const uint8_t *buf, uint8_t len)
{
    uint8_t cs = 0U;

    if (buf == NULL)
        return 0U;

    for (uint8_t i = 0U; i < len; i++)
        cs ^= buf[i];

    return cs;
}

HAL_StatusTypeDef BT_I2C_ReadMetrics(bt_i2c_metrics_t *m)
{
    if (m == NULL)
        return HAL_ERROR;

    HAL_StatusTypeDef ret = HAL_I2C_Master_Receive(&hi2c1,
                                                   (BT_I2C_SLAVE_ADDR_7BIT << 1),
                                                   g_bt_i2c_rx_frame,
                                                   BT_I2C_FRAME_SIZE,
                                                   BT_I2C_DEFAULT_TIMEOUT);

    if (ret != HAL_OK)
    {
        uint32_t err = HAL_I2C_GetError(&hi2c1);
        printf("[BT] I2C read error: ret=%d hal_err=0x%08lX\r\n",
               (int)ret, (unsigned long)err);

        m->available      = 0U;
        m->rssi           = -128;
        m->pkt_error_rate = 1000U;
        m->delay_ms         = 0U;
        m->jitter_ms      = 0U;
        return ret;
    }

    printf("[BT] raw:");
    for (uint8_t i = 0U; i < BT_I2C_FRAME_SIZE; i++)
        printf(" %02X", g_bt_i2c_rx_frame[i]);
    printf("\r\n");

    uint8_t cs = BT_I2C_CalcChecksum(g_bt_i2c_rx_frame, BT_I2C_FRAME_SIZE - 1U);
    if (g_bt_i2c_rx_frame[0] != 0xA5U || cs != g_bt_i2c_rx_frame[7])
    {
        printf("[BT] bad frame: hdr=0x%02X cs=0x%02X exp=0x%02X\r\n",
               g_bt_i2c_rx_frame[0],
               g_bt_i2c_rx_frame[7],
               cs);

        m->available      = 0U;
        m->rssi           = -128;
        m->pkt_error_rate = 1000U;
        m->delay_ms         = 0U;
        m->jitter_ms      = 0U;
        return HAL_ERROR;
    }

    m->available      = 1U;
    m->rssi           = (int8_t)g_bt_i2c_rx_frame[1];
    m->pkt_error_rate = ((uint16_t)g_bt_i2c_rx_frame[2] << 8) |
                         (uint16_t)g_bt_i2c_rx_frame[3];
    m->delay_ms         = ((uint16_t)g_bt_i2c_rx_frame[4] << 8) |
                         (uint16_t)g_bt_i2c_rx_frame[5];
    m->jitter_ms      = (uint16_t)g_bt_i2c_rx_frame[6];

    return HAL_OK;
}

bool BT_I2C_IsAvailable(const bt_i2c_metrics_t *m)
{
    if (m == NULL)
        return false;

    return (m->available != 0U);
}
