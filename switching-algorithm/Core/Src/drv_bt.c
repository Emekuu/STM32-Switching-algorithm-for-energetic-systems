#include "drv_bt.h"
#include "main.h"

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
    HAL_StatusTypeDef ret;

    if (m == NULL)
        return HAL_ERROR;

    ret = HAL_I2C_Master_Receive(&hi2c1,
                                 (BT_I2C_SLAVE_ADDR_7BIT << 1),
                                 g_bt_i2c_rx_frame,
                                 BT_I2C_FRAME_SIZE,
                                 BT_I2C_DEFAULT_TIMEOUT);

    if (ret != HAL_OK)
    {
        m->available = 0U;
        m->rssi = -128;
        m->pkt_error_rate = 1000U;
        return ret;
    }

    if (BT_I2C_CalcChecksum(g_bt_i2c_rx_frame, 4U) != g_bt_i2c_rx_frame[4])
    {
        m->available = 0U;
        m->rssi = -128;
        m->pkt_error_rate = 1000U;
        return HAL_ERROR;
    }

    m->available = (g_bt_i2c_rx_frame[3] & 0x01U) ? 1U : 0U;
    m->rssi = (int8_t)g_bt_i2c_rx_frame[1];
    m->pkt_error_rate = (uint16_t)g_bt_i2c_rx_frame[2];

    return HAL_OK;
}

bool BT_I2C_IsAvailable(const bt_i2c_metrics_t *m)
{
    if (m == NULL)
        return false;

    return (m->available != 0U);
}
