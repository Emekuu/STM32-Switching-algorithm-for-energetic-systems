#include "weighted_alg.h"

static float clipf(float x, float min, float max)
{
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

static float quality_rtt(const LinkMetrics *m, const NormalizationParams *n)
{
    if (n->rtt_max <= 0.0f) return 0.0f;
    float q = 1.0f - (m->rtt_ms / n->rtt_max);
    return clipf(q, 0.0f, 1.0f);
}

static float quality_loss(const LinkMetrics *m, const NormalizationParams *n)
{
    if (n->loss_max <= 0.0f) return 0.0f;
    float q = 1.0f - (m->loss / n->loss_max);
    return clipf(q, 0.0f, 1.0f);
}

static float quality_thr(const LinkMetrics *m, const NormalizationParams *n)
{
    if (n->thr_max <= 0.0f) return 0.0f;
    float q = m->throughput / n->thr_max;
    return clipf(q, 0.0f, 1.0f);
}

float link_score(const LinkMetrics *m,
                 const NormalizationParams *n,
                 const Weights *w)
{
    if (!m->available) {
        return 0.0f;
    }

    float qr  = quality_rtt(m, n);
    float ql  = quality_loss(m, n);
    float qth = quality_thr(m, n);

    return w->w_rtt  * qr
         + w->w_loss * ql
         + w->w_thr  * qth;
}

LinkId decide_link(SwitchState *state,
                   float score_eth,
                   float score_bt,
                   const SwitchParams *p,
                   uint32_t now_ms)
{
    if (state->current_link == LINK_ETH) {

        if (score_bt > score_eth + p->hysteresis) {
            if (state->better_since_ms == 0U) {
                state->better_since_ms = now_ms;
            } else if ((now_ms - state->better_since_ms) >= p->stable_time_ms) {
                state->current_link = LINK_BT;
                state->better_since_ms = 0U;
            }
        } else {
            state->better_since_ms = 0U;
        }

    } else { // current_link == LINK_BT

        if (score_eth > score_bt + p->hysteresis) {
            if (state->better_since_ms == 0U) {
                state->better_since_ms = now_ms;
            } else if ((now_ms - state->better_since_ms) >= p->stable_time_ms) {
                state->current_link = LINK_ETH;
                state->better_since_ms = 0U;
            }
        } else {
            state->better_since_ms = 0U;
        }
    }

    return state->current_link;
}
