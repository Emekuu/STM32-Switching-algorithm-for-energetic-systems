#ifndef RS485_IF_H
#define RS485_IF_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f7xx_hal.h"

typedef struct
{
    UART_HandleTypeDef *huart;
    GPIO_TypeDef *de_port;
    uint16_t de_pin;
    uint32_t timeout_ms;
    bool initialized;
} rs485_if_t;

HAL_StatusTypeDef RS485_Init(rs485_if_t *iface,
                             UART_HandleTypeDef *huart,
                             GPIO_TypeDef *de_port,
                             uint16_t de_pin,
                             uint32_t timeout_ms);

HAL_StatusTypeDef RS485_Send(rs485_if_t *iface,
                             const uint8_t *data,
                             uint16_t len);

HAL_StatusTypeDef RS485_Receive(rs485_if_t *iface,
                                uint8_t *data,
                                uint16_t len,
                                uint16_t *rx_len);

bool RS485_IsReady(const rs485_if_t *iface);

#endif /* RS485_IF_H */
