#ifndef APP_ETH_RX_TYPES_H_
#define APP_ETH_RX_TYPES_H_

#include "link_types.h"
#include <stdint.h>

#define JITTER_WINDOW_SIZE 32U
#define LOSS_WINDOW_SIZE   32U

#include "eth_parser.h"

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
    uint32_t rtt_ms;
    uint32_t bytes_in_window;
    uint32_t throughput_bps;
    uint32_t last_tp_tick_ms;
    uint32_t learnedintervalsum;
    uint8_t  learnedintervalcount;
    uint8_t  expectedvalid;
    uint32_t jitter_ms;
    uint32_t jitter_window[JITTER_WINDOW_SIZE];
    uint32_t jitter_sum;
    uint8_t  jitter_index;
    uint8_t  jitter_count;

    uint8_t  loss_window[LOSS_WINDOW_SIZE];
    uint32_t loss_window_sum;
    uint8_t  loss_window_index;
    uint8_t  loss_window_count;

    /* --- Reliability / periodicity tracking ---
     * Nastav t_expected_ms na ocakavany interval prijmu (napr. 1000 ms).
     * Nechaj 0 pre event-based alebo BT kanaly kde interval nie je pevny —
     * vtedy sa S1 (timing accuracy) ignoruje a skore zavisi len od loss+avail.
     */
    uint32_t t_expected_ms;        /* ocakavany inter-arrival interval [ms]; 0 = vypnute */
    uint32_t timing_error_sum;     /* suma |a(n) - t_expected_ms| v aktualnom okne */
    uint32_t timing_error_ms;      /* klzavy priemer timing error [ms] */
    uint8_t  timing_error_count;   /* pocet vzoriek v okne (max JITTER_WINDOW_SIZE) */
    uint32_t missed_intervals;     /* pocet intervalov kde a(n) > 1.5 * t_expected_ms */

    uint32_t reliability_score;    /* vypocitane reliability skore 0-1000 (promile) */
} rx_metrics_t;

#endif /* APP_ETH_RX_TYPES_H_ */
