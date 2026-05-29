#include "drv_rs485.h"
#include <stddef.h>

static void RS485_SetTxMode(rs485_if_t *iface)
{
    HAL_GPIO_WritePin(iface->de_port, iface->de_pin, GPIO_PIN_SET);
}

static void RS485_SetRxMode(rs485_if_t *iface)
{
    HAL_GPIO_WritePin(iface->de_port, iface->de_pin, GPIO_PIN_RESET);
}

HAL_StatusTypeDef RS485_Init(rs485_if_t *iface,
                             UART_HandleTypeDef *huart,
                             GPIO_TypeDef *de_port,
                             uint16_t de_pin,
                             uint32_t timeout_ms)
{
    if ((iface == NULL) || (huart == NULL) || (de_port == NULL))
        return HAL_ERROR;

    iface->huart = huart;
    iface->de_port = de_port;
    iface->de_pin = de_pin;
    iface->timeout_ms = timeout_ms;
    iface->initialized = true;

    RS485_SetRxMode(iface);
    return HAL_OK;
}

HAL_StatusTypeDef RS485_Send(rs485_if_t *iface,
                             const uint8_t *data,
                             uint16_t len)
{
    if ((iface == NULL) || (data == NULL) || (!iface->initialized))
        return HAL_ERROR;

    RS485_SetTxMode(iface);

    HAL_StatusTypeDef status = HAL_UART_Transmit(iface->huart,
                                                 (uint8_t *)data,
                                                 len,
                                                 iface->timeout_ms);

    if (status == HAL_OK)
    {
        while (__HAL_UART_GET_FLAG(iface->huart, UART_FLAG_TC) == RESET)
        {
        }
    }

    RS485_SetRxMode(iface);
    return status;
}

HAL_StatusTypeDef RS485_Receive(rs485_if_t *iface,
                                uint8_t *data,
                                uint16_t len,
                                uint16_t *rx_len)
{
    if ((iface == NULL) || (data == NULL) || (rx_len == NULL) || (!iface->initialized))
        return HAL_ERROR;

    RS485_SetRxMode(iface);

    HAL_StatusTypeDef status = HAL_UART_Receive(iface->huart,
                                                data,
                                                len,
                                                iface->timeout_ms);

    if (status == HAL_OK)
        *rx_len = len;
    else
        *rx_len = 0U;

    return status;
}

bool RS485_IsReady(const rs485_if_t *iface)
{
    if (iface == NULL)
        return false;

    return iface->initialized;
}
