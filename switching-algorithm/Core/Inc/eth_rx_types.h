#ifndef APP_ETH_RX_TYPES_H_
#define APP_ETH_RX_TYPES_H_

#include <stdint.h>
#include "link_types.h"
#include "eth_parser.h"

#define JITTER_WINDOW_SIZE 32U
#define LOSS_WINDOW_SIZE   32U

typedef struct
{
    uint8_t  initialized;
    uint32_t expected_seq;
    uint32_t received_packets;
    uint32_t lost_packets;
    uint32_t out_of_order_packets;
    uint32_t duplicate_packets;

    uint32_t last_rx_tick_ms;

    int32_t  delay_raw_ms;
    int32_t  last_delay_raw_ms;
    uint8_t  last_delay_raw_valid;

    uint32_t delay_rel_ms;

    uint32_t bytes_in_window;
    uint32_t throughput_bps;
    uint32_t last_tp_tick_ms;

    uint32_t jitter_ms;
    uint32_t jitter_window[JITTER_WINDOW_SIZE];
    uint32_t jitter_sum;
    uint8_t  jitter_index;
    uint8_t  jitter_count;

    uint8_t  loss_window[LOSS_WINDOW_SIZE];
    uint32_t loss_window_sum;
    uint8_t  loss_window_index;
    uint8_t  loss_window_count;
} rx_metrics_t;

#endif /* APP_ETH_RX_TYPES_H_ */
