#include "eth_metrics.h"
#include <string.h>
#include <stdio.h>

/* =========================================================================
 * Interne pomocne funkcie — klzave okna
 * ========================================================================= */
#define METRICS_LEARN_MIN_INTERVAL_MS      100U
#define METRICS_LEARN_SAMPLES              3U
#define METRICS_EXPECTED_ALPHA_NUM         9U
#define METRICS_EXPECTED_ALPHA_DEN         10U
#define METRICS_EXPECTED_UPDATE_MIN_PCT    70U
#define METRICS_EXPECTED_UPDATE_MAX_PCT    130U

static uint8_t MetricsIntervalLooksNormal(uint32_t ia, uint32_t expected);
static void Metrics_JitterWindowPush(rx_metrics_t *m, uint32_t variation_ms)

{
    if (m == NULL)
        return;

    if (m->jitter_count < JITTER_WINDOW_SIZE)
    {
        m->jitter_window[m->jitter_index] = variation_ms;
        m->jitter_sum += variation_ms;
        m->jitter_count++;
    }
    else
    {
        m->jitter_sum -= m->jitter_window[m->jitter_index];
        m->jitter_window[m->jitter_index] = variation_ms;
        m->jitter_sum += variation_ms;
    }

    m->jitter_index++;
    if (m->jitter_index >= JITTER_WINDOW_SIZE)
        m->jitter_index = 0U;

    if (m->jitter_count > 0U)
        m->jitter_ms = m->jitter_sum / m->jitter_count;
    else
        m->jitter_ms = 0U;
}

static void Metrics_LossWindowPush(rx_metrics_t *m, uint8_t lost)
{
    if (m == NULL)
        return;

    if (m->loss_window_count < LOSS_WINDOW_SIZE)
    {
        m->loss_window[m->loss_window_index] = lost;
        m->loss_window_sum += lost;
        m->loss_window_count++;
    }
    else
    {
        m->loss_window_sum -= m->loss_window[m->loss_window_index];
        m->loss_window[m->loss_window_index] = lost;
        m->loss_window_sum += lost;
    }

    m->loss_window_index++;
    if (m->loss_window_index >= LOSS_WINDOW_SIZE)
        m->loss_window_index = 0U;
}

/*
 * Metrics_TimingWindowPush
 * Klzavy priemer |a(n) - T_expected| — rovnaka velkost okna ako jitter.
 * Vola sa len ak t_expected_ms != 0.
 */
static void Metrics_TimingWindowPush(rx_metrics_t *m, uint32_t abs_error_ms)
{
    if (m == NULL)
        return;

    if (m->timing_error_count < JITTER_WINDOW_SIZE)
    {
        m->timing_error_sum += abs_error_ms;
        m->timing_error_count++;
    }
    else
    {
        /* Odober stary prispevok: aproximacia cez aktualny priemer.
         * Presnejsie by bolo samostatne okno, ale setri RAM na MCU. */
        m->timing_error_sum -= m->timing_error_ms;
        m->timing_error_sum += abs_error_ms;
    }

    if (m->timing_error_count > 0U)
        m->timing_error_ms = m->timing_error_sum / m->timing_error_count;
    else
        m->timing_error_ms = 0U;
}

/* =========================================================================
 * Verejne API
 * ========================================================================= */

void Metrics_Reset(rx_metrics_t* m)
{
    if (m == NULL)
    {
        return;
    }

    {
        uint32_t savedtexpected = m->t_expected_ms;
        uint8_t savedexpectedvalid = m->expectedvalid;

        memset(m, 0, sizeof(*m));

        m->t_expected_ms = savedtexpected;
        m->expectedvalid = savedexpectedvalid;
    }
}

void Metrics_OnFrameReceived(rx_metrics_t *m,
                             const generic_frame_info_t *f,
                             uint32_t rx_tick_ms)
{
    printf("[DBG_RTT] seq=%lu has_ts=%u tx=%lu rx=%lu\n",
           (unsigned long)f->seq,
           (unsigned)f->has_tx_ts,
           (unsigned long)f->tx_ts_ms,
           (unsigned long)rx_tick_ms);

    if ((m == NULL) || (f == NULL))
        return;

    /* has_tx_ts pouzivame len ak su casove zakladne synchronizovane.
     * Ak je rozdiel rx - tx < -60 s, server bezi dlhsie ako F7 → ignoruj ts. */
    uint8_t ts_usable = 0U;
    if (f->has_tx_ts)
    {
        int32_t ts_diff = (int32_t)rx_tick_ms - (int32_t)f->tx_ts_ms;
        ts_usable = (ts_diff > -60000) ? 1U : 0U;

        if (ts_usable)
        {
            int32_t rtt = ts_diff;
            if (rtt < 0) rtt = 0;
            m->rtt_ms = (uint32_t)rtt;
        }

        printf("[DBG_RTT2] seq=%lu rtt=%lu ts_usable=%u\n",
               (unsigned long)f->seq,
               (unsigned long)m->rtt_ms,
               (unsigned)ts_usable);
    }

    /* --- Prva vzorka: inicializacia stavu --- */
    if (!m->initialized)
    {
        uint32_t saved_t_expected = m->t_expected_ms;  /* zachovaj pred memset */
        memset(m, 0, sizeof(*m));
        m->t_expected_ms     = saved_t_expected;        /* obnov po memset */
        m->initialized       = 1U;
        m->expected_seq      = f->seq + 1U;
        m->received_packets  = 1U;
        m->last_rx_tick_ms   = rx_tick_ms;
        m->last_tp_tick_ms   = rx_tick_ms;
        m->bytes_in_window   = f->payload_len;

        if (f->has_tx_ts && ((int32_t)rx_tick_ms - (int32_t)f->tx_ts_ms > -60000))
            m->last_tx_ts_ms = f->tx_ts_ms;

        Metrics_LossWindowPush(m, 0U);
        return;
    }

    /* --- Sequence tracking --- */
    if (f->seq == m->expected_seq)
    {
        m->received_packets++;
        m->expected_seq++;
        Metrics_LossWindowPush(m, 0U);
    }
    else if (f->seq > m->expected_seq)
    {
        uint32_t gap = f->seq - m->expected_seq;
        m->lost_packets += gap;

        for (uint32_t i = 0U; i < gap; i++)
            Metrics_LossWindowPush(m, 1U);

        m->received_packets++;
        m->expected_seq = f->seq + 1U;
        Metrics_LossWindowPush(m, 0U);
    }
    else
    {
        if (f->seq + 1U == m->expected_seq)
            m->duplicate_packets++;
        else
            m->out_of_order_packets++;

        return;
    }

    /* --- Inter-arrival a jitter --- */
    uint32_t interarrival = rx_tick_ms - m->last_rx_tick_ms;

    if (ts_usable && (m->last_tx_ts_ms != 0U))
    {
        uint32_t rx_delta = interarrival;
        uint32_t tx_delta = f->tx_ts_ms - m->last_tx_ts_ms;

        int32_t variation = (int32_t)rx_delta - (int32_t)tx_delta;
        if (variation < 0)
            variation = -variation;

        Metrics_JitterWindowPush(m, (uint32_t)variation);

        m->rtt_ms        = rx_delta;
        if (ts_usable)
            m->last_tx_ts_ms = f->tx_ts_ms;
    }
    else
    {
        Metrics_JitterWindowPush(m, interarrival);
        m->rtt_ms = interarrival;
    }
    /* --- Expected interval auto-learn / adapt --- */
    if (m->t_expected_ms == 0U || m->expectedvalid == 0U)
    {
        if (interarrival >= METRICS_LEARN_MIN_INTERVAL_MS)
        {
            m->learnedintervalsum += interarrival;

            if (m->learnedintervalcount < 255U)
            {
                m->learnedintervalcount++;
            }

            m->t_expected_ms = m->learnedintervalsum / m->learnedintervalcount;

            printf("RXTIM learn ia=%lu ms texp=%lu ms samples=%u\r\n",
                   (unsigned long)interarrival,
                   (unsigned long)m->t_expected_ms,
                   (unsigned)m->learnedintervalcount);

            if (m->learnedintervalcount >= METRICS_LEARN_SAMPLES)
            {
                m->expectedvalid = 1U;
                printf("RXTIM lock texp=%lu ms\r\n",
                       (unsigned long)m->t_expected_ms);
            }
        }
    }
    else
    {
        int32_t terr = (int32_t)interarrival - (int32_t)m->t_expected_ms;

        if (terr < 0)
        {
            terr = -terr;
        }

        Metrics_TimingWindowPush(m, (uint32_t)terr);

        if (interarrival > ((m->t_expected_ms * 3U) / 2U))
        {
            m->missed_intervals++;

            printf("RXTIM miss ia=%lu ms texp=%lu ms terr=%lu ms missed=%lu\r\n",
                   (unsigned long)interarrival,
                   (unsigned long)m->t_expected_ms,
                   (unsigned long)terr,
                   (unsigned long)m->missed_intervals);
        }
        else
        {
            if (MetricsIntervalLooksNormal(interarrival, m->t_expected_ms))
            {
                m->t_expected_ms =
                    ((m->t_expected_ms * METRICS_EXPECTED_ALPHA_NUM) + interarrival) /
                    METRICS_EXPECTED_ALPHA_DEN;
            }

            printf("RXTIM ok ia=%lu ms texp=%lu ms terr=%lu ms missed=%lu\r\n",
                   (unsigned long)interarrival,
                   (unsigned long)m->t_expected_ms,
                   (unsigned long)terr,
                   (unsigned long)m->missed_intervals);
        }
    }
    /* --- Timing accuracy (len ak je nastaveny ocakavany interval) --- */


    m->last_rx_tick_ms = rx_tick_ms;

    /* --- Throughput --- */
    m->bytes_in_window += f->payload_len;

    if ((rx_tick_ms - m->last_tp_tick_ms) >= 1000U)
    {
        m->throughput_bps  = m->bytes_in_window * 8U;
        m->bytes_in_window = 0U;
        m->last_tp_tick_ms = rx_tick_ms;
    }
}

/* =========================================================================
 * Gettery
 * ========================================================================= */
static uint8_t MetricsIntervalLooksNormal(uint32_t ia, uint32_t expected)
{
    uint32_t minv;
    uint32_t maxv;

    if (expected == 0U)
    {
        return 0U;
    }

    minv = (expected * METRICS_EXPECTED_UPDATE_MIN_PCT) / 100U;
    maxv = (expected * METRICS_EXPECTED_UPDATE_MAX_PCT) / 100U;

    return (ia >= minv && ia <= maxv) ? 1U : 0U;
}
uint32_t Metrics_GetLossPercent(const rx_metrics_t *m)
{
    if ((m == NULL) || (m->loss_window_count == 0U))
        return 0U;

    return (m->loss_window_sum * 100U) / m->loss_window_count;
}

uint32_t Metrics_GetJitterMs(const rx_metrics_t *m)
{
    if (m == NULL)
        return 0U;

    return m->jitter_ms;
}

uint32_t Metrics_GetThroughputBps(const rx_metrics_t *m)
{
    if (m == NULL)
        return 0U;

    return m->throughput_bps;
}

uint32_t MetricsGetTimingErrorMs(const rx_metrics_t* m)
{
    if (m == NULL)
    {
        return 0U;
    }
    return m->timing_error_ms;
}

uint32_t MetricsGetMissedIntervals(const rx_metrics_t* m)
{
    if (m == NULL)
    {
        return 0U;
    }
    return m->missed_intervals;
}
/* =========================================================================
 * Metrics_CalcReliability
 *
 * Vrati 0-1000 (promile).  Vahy musia dat spolu != 0.
 *
 * S1 (timing) : ignorovane ak t_expected_ms == 0  → nahradi sa hodnotou 1000
 * S2 (loss)   : 1000 - loss_permille
 * S3 (avail)  : pomer prijatych intervalov bez missed voci celku
 * ========================================================================= */
uint32_t Metrics_CalcReliability(const rx_metrics_t *m,
                                 uint8_t w_timing,
                                 uint8_t w_loss,
                                 uint8_t w_avail)
{
    if ((m == NULL) || (!m->initialized))
        return 0U;

    uint32_t total_w = (uint32_t)w_timing + (uint32_t)w_loss + (uint32_t)w_avail;
    if (total_w == 0U)
        return 0U;

    /* S1 — Timing accuracy */
    uint32_t s1 = 1000U;   /* default: plny bod ak sa netracuje */
    if ((m->t_expected_ms > 0U) && (m->timing_error_count > 0U))
    {
        uint32_t rel_err_promille =
            (m->timing_error_ms * 1000U) / m->t_expected_ms;
        s1 = (rel_err_promille >= 1000U) ? 0U : (1000U - rel_err_promille);
    }

    /* S2 — Packet loss (GetLossPercent vracia 0-100, prevediem na promile) */
    uint32_t loss_promille = Metrics_GetLossPercent(m) * 10U;
    uint32_t s2 = (loss_promille >= 1000U) ? 0U : (1000U - loss_promille);

    /* S3 — Availability cez missed intervals */
    uint32_t s3 = 1000U;
    if (m->t_expected_ms > 0U)
    {
        uint32_t total_intervals =
            m->received_packets + m->lost_packets + m->missed_intervals;
        if (total_intervals > 0U)
        {
            uint32_t miss_promille =
                (m->missed_intervals * 1000U) / total_intervals;
            s3 = (miss_promille >= 1000U) ? 0U : (1000U - miss_promille);
        }
    }

    return (s1 * (uint32_t)w_timing +
            s2 * (uint32_t)w_loss   +
            s3 * (uint32_t)w_avail) / total_w;
}

/* =========================================================================
 * Metrics_ApplyToLink
 * ========================================================================= */
void Metrics_ApplyToLink(link_info_t *link, const rx_metrics_t *m)
{
    if ((link == NULL) || (m == NULL))
        return;

    link->enabled              = true;
    link->metrics.security_ok  = true;
    link->metrics.signal_dbm   = 0;

    if (!m->initialized)
    {
        /* Ak measurement_valid je true, data boli nastavene priamo (napr. stub).
         * Vypocitaj reliability zo skutocnych link->metrics hodnot. */
        if (link->metrics.measurement_valid)
        {
            uint32_t loss_p = link->metrics.packet_loss_permille / 10U; /* permille → percent */
            uint32_t s2 = (loss_p >= 100U) ? 0U : (1000U - loss_p * 10U);
            link->metrics.reliability_score = s2;
        }
        else
        {
            link->metrics.measurement_valid      = false;
            link->state                          = LINK_STATE_DOWN;
            link->metrics.rtt_ms                 = 1000U;
            link->metrics.jitter_ms              = 300U;
            link->metrics.packet_loss_permille   = 100U;
            link->metrics.reliability_score      = 0U;
        }
        return;
    }

    link->metrics.measurement_valid    = true;
    link->metrics.rtt_ms               = m->rtt_ms;
    link->metrics.jitter_ms            = Metrics_GetJitterMs(m);
    link->metrics.packet_loss_permille = Metrics_GetLossPercent(m);

    if (link->metrics.packet_loss_permille >= link->thresholds.threshold_loss_percent)
        link->state = LINK_STATE_DEGRADED;
    else
        link->state = LINK_STATE_UP;

    /* --- Reliability score ---
     * Vahy podla typu linky:
     *   ETH_DIRECT / ETH_PLC : periodicke merania — timing je dolezity
     *   BLUETOOTH             : t_expected=0, S1 sa ignoruje, vaha 0
     *   RS485 / LINK_NONE     : event-based alebo neznamy, timing ignorovat
     */
    uint8_t wt, wl, wa;

    switch (link->type)
    {
        case LINK_ETH_DIRECT:
        case LINK_ETH_PLC:
            wt = 4U; wl = 4U; wa = 2U;
            break;

        case LINK_BLUETOOTH:
            /* t_expected_ms je 0 → S1 = 1000 (neutralne), ale vaha 0 ho vyluca uplne */
            wt = 0U; wl = 6U; wa = 4U;
            break;

        case LINK_RS485:
            /* Polling/event — cas prichodu nezaruceny, availability a loss su dolezite */
            wt = 0U; wl = 5U; wa = 5U;
            break;

        default:
            wt = 3U; wl = 4U; wa = 3U;
            break;
    }

    link->metrics.reliability_score =
        Metrics_CalcReliability(m, wt, wl, wa);
}

/* =========================================================================
 * Metrics_IsTimedOut / Metrics_Age
 * ========================================================================= */
uint8_t Metrics_IsTimedOut(const rx_metrics_t *m,
                           uint32_t now_ms,
                           uint32_t timeout_ms)
{
    if ((m == NULL) || !m->initialized)
        return 1U;

    return ((now_ms - m->last_rx_tick_ms) >= timeout_ms) ? 1U : 0U;
}

void Metrics_Age(rx_metrics_t *m, uint32_t now_ms, uint32_t timeout_ms)
{
    if (m == NULL)
        return;

    if (!m->initialized)
        return;

    if ((now_ms - m->last_rx_tick_ms) < timeout_ms)
        return;

    m->throughput_bps = 0U;
    m->jitter_ms      = 0U;

    /* Simuluj plny loss v okne pri timeout */
    m->loss_window_sum = m->loss_window_count;
}
uint32_t MetricsGetExpectedIntervalMs(const rx_metrics_t* m)
{
    if (m == NULL)
    {
        return 0U;
    }
    return m->t_expected_ms;
}

uint32_t MetricsGet_timing_error_ms(const rx_metrics_t* m)
{
    if (m == NULL)
    {
        return 0U;
    }
    return m-> timing_error_ms;
}

uint32_t MetricsGetmissed_intervals(const rx_metrics_t* m)
{
    if (m == NULL)
    {
        return 0U;
    }
    return m->missed_intervals;
}

uint8_t MetricsHasExpectedInterval(const rx_metrics_t* m)
{
    if (m == NULL)
    {
        return 0U;
    }
    return m->expectedvalid;
}
