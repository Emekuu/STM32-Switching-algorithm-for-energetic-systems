#ifndef BTMETRICS_H
#define BTMETRICS_H

#include <stdint.h>

typedef struct __attribute__((packed))
{
    uint8_t  header;
    int8_t   rssi;
    uint16_t loss_permille;
    uint16_t rtt_ms;
    uint16_t jitter_ms;
    uint8_t  reserved[3];
    uint8_t  checksum;
} BLEMetricsFrame_t;

#endif /* BTMETRICS_H */
