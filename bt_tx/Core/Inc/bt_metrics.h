#ifndef BLE_METRICS_H
#define BLE_METRICS_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint8_t  header;          // 0xA5
    int8_t   rssi;            // dBm
    uint16_t loss_permille;   // 0..1000
    uint16_t rtt_ms;
    uint8_t  jitter_ms;
    uint8_t  checksum;        // XOR byte0..byte6
} ble_metrics_frame_t;

static inline uint8_t BLE_MetricsChecksum(const uint8_t *buf, uint8_t len)
{
    uint8_t cs = 0U;
    for (uint8_t i = 0U; i < len; i++)
        cs ^= buf[i];
    return cs;
}

#endif
