#ifndef ETH_RX_TYPES_H
#define ETH_RX_TYPES_H
#include "link_types.h"
#include <stdint.h>

#define JITTER_WINDOW_SIZE 32U
#define LOSS_WINDOW_SIZE   32U


typedef struct
{
    uint32_t seq;
    uint32_t tx_ts_ms;
    uint16_t payload_len;
    uint8_t  path_id;
    uint8_t  has_tx_ts;
} generic_frame_info_t;

typedef struct
{
    uint8_t  initialized;
    uint32_t expected_seq;
    uint32_t received_packets;
    uint32_t lost_packets;
    uint32_t out_of_order_packets;
    uint32_t duplicate_packets;

    uint32_t last_rx_tick_ms;
    uint32_t last_tx_ts_ms;

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

#endif /* ETH_RX_TYPES_H */
