#ifndef DRV_BT_H
#define DRV_BT_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t  available;
    int8_t   rssi;
    uint16_t pkt_error_rate;
    uint16_t delay_ms;
    uint16_t jitter_ms;
} bt_i2c_metrics_t;

HAL_StatusTypeDef BT_I2C_ReadMetrics(bt_i2c_metrics_t *m);
bool BT_I2C_IsAvailable(const bt_i2c_metrics_t *m);

#endif
