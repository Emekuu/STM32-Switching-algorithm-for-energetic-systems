
#ifndef LINK_SELECTION_H
#define LINK_SELECTION_H

#include <stdint.h>

typedef struct {
    float rtt_ms;        // RTT
    float loss;
    float throughput;    //
    uint8_t available;   // 0 = down, 1 = up
} LinkMetrics;

typedef enum {
    LINK_ETH = 0,
    LINK_BT  = 1
} LinkId;

typedef struct {
    float rtt_max;
    float loss_max;
    float thr_max;
} NormalizationParams;

typedef struct {
    float w_rtt;
    float w_loss;
    float w_thr;
} Weights;

typedef struct {
    float hysteresis;
    uint32_t stable_time_ms;
} SwitchParams;

typedef struct {
    LinkId current_link;
    uint32_t better_since_ms;
} SwitchState;

float link_score(const LinkMetrics *m,
                 const NormalizationParams *n,
                 const Weights *w);

LinkId decide_link(SwitchState *state,
                   float score_eth,
                   float score_bt,
                   const SwitchParams *p,
                   uint32_t now_ms);

#endif // LINK_SELECTION_H

