#ifndef BT_I2C_IF_H
#define BT_I2C_IF_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f7xx_hal.h"

#define BT_I2C_FRAME_SIZE        5U
#define BT_I2C_DEFAULT_TIMEOUT   100U
#define BT_I2C_SLAVE_ADDR_7BIT   0x28U

typedef struct
{
    uint8_t available;
    int8_t rssi;
    uint16_t pkt_error_rate;
} bt_i2c_metrics_t;

HAL_StatusTypeDef BT_I2C_ReadMetrics(bt_i2c_metrics_t *m);
bool BT_I2C_IsAvailable(const bt_i2c_metrics_t *m);

#endif /* BT_I2C_IF_H */
