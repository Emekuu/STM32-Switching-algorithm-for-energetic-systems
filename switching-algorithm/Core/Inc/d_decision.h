#ifndef DECISION_H
#define DECISION_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "link_types.h"

#define DECISION_MAX_LINKS ((uint32_t)LINK_TYPE_COUNT)

typedef enum
{
    SECURITY_OFF = 0,
    SECURITY_PREFERRED,
    SECURITY_REQUIRED
} security_policy_t;

typedef enum
{
    DECISION_ALG_LOWEST_LATENCY = 0,
    DECISION_ALG_WEIGHTED_SCORE,
    DECISION_ALG_SECURITY_FIRST,
	DECISION_ALG_THRESHOLD_WEIGHTED
} decision_algorithm_t;

typedef struct
{
    decision_algorithm_t algorithm;
    security_policy_t security_policy;

    uint32_t max_rtt_ms;
    uint32_t max_jitter_ms;
    uint32_t max_loss_permille;

    int32_t weight_rtt;
    int32_t weight_jitter;
    int32_t weight_loss;
    int32_t weight_signal;
    int32_t weight_security;

    uint32_t threshold_rtt_ms;
    uint32_t threshold_jitter_ms;
    uint32_t threshold_loss_permille;

    uint32_t recovery_rtt_ms;
    uint32_t recovery_jitter_ms;
    uint32_t recovery_loss_permille;

    int32_t switch_hysteresis_score;
    uint32_t min_switch_interval_ms;
} decision_config_t;

typedef struct
{
    bool valid;
    link_type_t selected_link;
    int32_t selected_score;
} decision_result_t;

decision_result_t decision_select_link(const link_info_t *links,
                                       uint32_t link_count,
                                       const decision_config_t *config);

#endif /* DECISION_H */
