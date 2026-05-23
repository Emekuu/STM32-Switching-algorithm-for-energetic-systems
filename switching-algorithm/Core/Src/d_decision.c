
#include <stddef.h>
#include <stdbool.h>
#include <limits.h>


#include "d_decision.h"



static bool decision_link_basic_valid(const link_info_t *link)
{
    if (link == NULL)
        return false;

    if (!link->enabled)
        return false;

    if (link->state == LINK_STATE_DOWN)
        return false;

    if (!link->metrics.measurement_valid)
        return false;

    return true;
}

static bool decision_link_security_allowed(const link_info_t *link,
                                           security_policy_t policy)
{
    if (policy == SECURITY_OFF)
        return true;

    if (policy == SECURITY_PREFERRED)
        return true;

    if (policy == SECURITY_REQUIRED)
        return link->metrics.security_ok;

    return true;
}

static bool decision_link_threshold_ok(const link_info_t *link,
                                       const decision_config_t *config)
{
    if ((link == NULL) || (config == NULL))
        return false;

    if (link->metrics.rtt_ms > config->threshold_rtt_ms)
        return false;

    if (link->metrics.jitter_ms > config->threshold_jitter_ms)
        return false;

    if (link->metrics.packet_loss_permille > config->threshold_loss_permille)
        return false;

    return true;
}

static int32_t decision_score_rtt(uint32_t rtt_ms, uint32_t max_rtt_ms)
{
    if (max_rtt_ms == 0U)
        return 0;

    if (rtt_ms >= max_rtt_ms)
        return 0;

    return (int32_t)((1000U * (max_rtt_ms - rtt_ms)) / max_rtt_ms);
}

static int32_t decision_score_jitter(uint32_t jitter_ms, uint32_t max_jitter_ms)
{
    if (max_jitter_ms == 0U)
        return 0;

    if (jitter_ms >= max_jitter_ms)
        return 0;

    return (int32_t)((1000U * (max_jitter_ms - jitter_ms)) / max_jitter_ms);
}

static int32_t decision_score_loss(uint32_t loss_permille, uint32_t max_loss_permille)
{
    if (max_loss_permille == 0U)
        return 0;

    if (loss_permille >= max_loss_permille)
        return 0;

    return (int32_t)((1000U * (max_loss_permille - loss_permille)) / max_loss_permille);
}

static int32_t decision_score_signal(int16_t signal_dbm)
{
    if (signal_dbm >= -50)
        return 1000;
    if (signal_dbm <= -100)
        return 0;

    return (int32_t)((signal_dbm + 100) * 20);
}

static int32_t decision_calculate_weighted_score(const link_info_t *link,
                                                 const decision_config_t *config)
{
    int32_t score = 0;

    score += decision_score_rtt(link->metrics.rtt_ms, config->max_rtt_ms) *
             config->weight_rtt;

    score += decision_score_jitter(link->metrics.jitter_ms, config->max_jitter_ms) *
             config->weight_jitter;

    score += decision_score_loss(link->metrics.packet_loss_permille,
                                 config->max_loss_permille) *
             config->weight_loss;

    score += decision_score_signal(link->metrics.signal_dbm) *
             config->weight_signal;

    if (link->metrics.security_ok)
        score += 1000 * config->weight_security;

    if (link->state == LINK_STATE_DEGRADED)
        score -= 500;

    return score;
}

static decision_result_t decision_select_threshold_weighted(const link_info_t *links,
                                                            uint32_t link_count,
                                                            const decision_config_t *config)
{
    decision_result_t result = { false, LINK_NONE, 0 };
    int32_t best_score = (-2147483647 - 1);

    for (uint32_t i = 0; i < link_count; i++)
    {
        if (!decision_link_basic_valid(&links[i]))
            continue;

        if (!decision_link_security_allowed(&links[i], config->security_policy))
            continue;

        if (!decision_link_threshold_ok(&links[i], config))
            continue;

        int32_t score = decision_calculate_weighted_score(&links[i], config);

        if ((!result.valid) || (score > best_score))
        {
            best_score = score;
            result.valid = true;
            result.selected_link = links[i].type;
            result.selected_score = score;
        }
    }

    return result;
}


static decision_result_t decision_select_lowest_latency(const link_info_t *links,
                                                        uint32_t link_count,
                                                        const decision_config_t *config)
{
    (void)config;

    decision_result_t result = { false, LINK_NONE, 0 };
    uint32_t best_rtt = 0xFFFFFFFFU;

    for (uint32_t i = 0; i < link_count; i++)
    {
        if (!decision_link_basic_valid(&links[i]))
            continue;

        if (!decision_link_security_allowed(&links[i], config->security_policy))
            continue;

        if (links[i].metrics.rtt_ms < best_rtt)
        {
            best_rtt = links[i].metrics.rtt_ms;
            result.valid = true;
            result.selected_link = links[i].type;
            result.selected_score = (int32_t)(100000U - best_rtt);
        }
    }

    return result;
}

static decision_result_t decision_select_weighted_score(const link_info_t *links,
                                                        uint32_t link_count,
                                                        const decision_config_t *config)
{
    decision_result_t result = { false, LINK_NONE, 0 };
    int32_t best_score = INT_MIN;


    for (uint32_t i = 0; i < link_count; i++)
    {
        if (!decision_link_basic_valid(&links[i]))
            continue;

        if (!decision_link_security_allowed(&links[i], config->security_policy))
            continue;

        int32_t score = decision_calculate_weighted_score(&links[i], config);

        if ((!result.valid) || (score > best_score))
        {
            best_score = score;
            result.valid = true;
            result.selected_link = links[i].type;
            result.selected_score = score;
        }
    }

    return result;
}



static decision_result_t decision_select_security_first(const link_info_t *links,
                                                        uint32_t link_count,
                                                        const decision_config_t *config)
{
    decision_result_t secure_best = { false, LINK_NONE, 0 };
    int32_t best_secure_score = INT_MIN;

    for (uint32_t i = 0; i < link_count; i++)
    {
        if (!decision_link_basic_valid(&links[i]))
            continue;

        if (!links[i].metrics.security_ok)
            continue;

        int32_t score = decision_calculate_weighted_score(&links[i], config);

        if ((!secure_best.valid) || (score > best_secure_score))
        {
            best_secure_score = score;
            secure_best.valid = true;
            secure_best.selected_link = links[i].type;
            secure_best.selected_score = score;
        }
    }

    if (secure_best.valid)
        return secure_best;

    if (config->security_policy == SECURITY_REQUIRED)
        return secure_best;

    return decision_select_weighted_score(links, link_count, config);
}

decision_result_t decision_select_link(const link_info_t *links,
                                       uint32_t link_count,
                                       const decision_config_t *config)
{
    decision_result_t invalid_result = { false, LINK_NONE, 0 };

    if ((links == NULL) || (config == NULL) || (link_count == 0U) || (link_count > DECISION_MAX_LINKS))
        return invalid_result;

    switch (config->algorithm)
    {
        case DECISION_ALG_LOWEST_LATENCY:
            return decision_select_lowest_latency(links, link_count, config);

        case DECISION_ALG_WEIGHTED_SCORE:
            return decision_select_weighted_score(links, link_count, config);

        case DECISION_ALG_SECURITY_FIRST:
            return decision_select_security_first(links, link_count, config);

        case DECISION_ALG_THRESHOLD_WEIGHTED:
            return decision_select_threshold_weighted(links, link_count, config);

        default:
            return invalid_result;
    }
}
