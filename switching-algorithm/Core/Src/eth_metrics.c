#include "eth_metrics.h"
#include <string.h>
#include <stdio.h>

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

void Metrics_Reset(rx_metrics_t *m)
{
    if (m == NULL)
        return;

    memset(m, 0, sizeof(*m));
}
void Metrics_OnFrameReceived(rx_metrics_t *m,
                             const generic_frame_info_t *f,
                             uint32_t rx_tick_ms)
{
    if ((m == NULL) || (f == NULL))
        return;

    int32_t sample_ms = 0;
    uint8_t sample_valid = 0U;

    if (f->has_tx_ts)
    {
        sample_ms = (int32_t)rx_tick_ms - (int32_t)f->tx_ts_ms;
        sample_valid = 1U;

        printf("[DBG_DELAY] seq=%lu tx=%lu rx=%lu raw_diff=%ld\r\n",
               (unsigned long)f->seq,
               (unsigned long)f->tx_ts_ms,
               (unsigned long)rx_tick_ms,
               (long)sample_ms);
    }
    else if (m->initialized)
    {
        sample_ms = (int32_t)(rx_tick_ms - m->last_rx_tick_ms); /* inter-arrival time */
        sample_valid = 1U;

        printf("[DBG_IAT] seq=%lu rx=%lu iat=%ld\r\n",
               (unsigned long)f->seq,
               (unsigned long)rx_tick_ms,
               (long)sample_ms);
    }

    if (!m->initialized)
    {
        memset(m, 0, sizeof(*m));
        m->initialized = 1U;
        m->expected_seq = f->seq + 1U;
        m->received_packets = 1U;
        m->last_rx_tick_ms = rx_tick_ms;
        m->last_tp_tick_ms = rx_tick_ms;
        m->bytes_in_window = f->payload_len;

        if (f->has_tx_ts)
        {
            m->delay_raw_ms = (int32_t)rx_tick_ms - (int32_t)f->tx_ts_ms;
            m->last_delay_raw_ms = m->delay_raw_ms;
            m->last_delay_raw_valid = 1U;
        }
        else
        {
            m->delay_raw_ms = 0;
            m->last_delay_raw_ms = 0;
            m->last_delay_raw_valid = 0U;
        }

        m->delay_rel_ms = 0U;
        m->jitter_ms = 0U;

        Metrics_LossWindowPush(m, 0U);
        return;
    }

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

    if (sample_valid)
    {
        m->delay_raw_ms = sample_ms;

        if (m->last_delay_raw_valid)
        {
            int32_t rel = sample_ms - m->last_delay_raw_ms;
            if (rel < 0)
                rel = -rel;

            m->delay_rel_ms = (uint32_t)rel;
            Metrics_JitterWindowPush(m, m->delay_rel_ms);
        }
        else
        {
            m->delay_rel_ms = 0U;
        }

        m->last_delay_raw_ms = sample_ms;
        m->last_delay_raw_valid = 1U;
    }

    m->last_rx_tick_ms = rx_tick_ms;
    m->bytes_in_window += f->payload_len;

    {
        uint32_t dt_ms = rx_tick_ms - m->last_tp_tick_ms;
        if (dt_ms >= 900U)
        {
            if (m->bytes_in_window > 0U)
            {
                uint64_t bits = (uint64_t)m->bytes_in_window * 8ULL;
                uint64_t tp = (bits * 1000ULL) / dt_ms;
                m->throughput_bps = (tp > 0xFFFFFFFFULL) ? 0xFFFFFFFFU : (uint32_t)tp;
            }
            else
            {
                m->throughput_bps = 0U;
            }

            m->bytes_in_window = 0U;
            m->last_tp_tick_ms = rx_tick_ms;
        }
    }
}
uint32_t Metrics_GetLossPercent(const rx_metrics_t *m)
{
    if ((m == NULL) || (m->loss_window_count == 0U))
        return 0U;

    return (m->loss_window_sum * 100U) / m->loss_window_count;
}

uint32_t Metrics_GetLossPermille(const rx_metrics_t *m)
{
    if ((m == NULL) || (m->loss_window_count == 0U))
        return 0U;

    return (m->loss_window_sum * 1000U) / m->loss_window_count;
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


int32_t Metrics_GetDelayRawMs(const rx_metrics_t *m)
{
    if (m == NULL)
        return 0;
    return m->delay_raw_ms;
}

uint32_t Metrics_GetDelayRelMs(const rx_metrics_t *m)
{
    if (m == NULL)
        return 0U;
    return m->delay_rel_ms;
}

void Metrics_ApplyToLink(link_info_t *link, const rx_metrics_t *m)
{
    if ((link == NULL) || (m == NULL))
        return;

    link->enabled = true;
    link->metrics.security_ok = true;
    link->metrics.signal_dbm = 0;

    if (!m->initialized)
    {
        link->metrics.measurement_valid = false;
        link->state = LINK_STATE_DOWN;
        link->metrics.delay_ms = 1000U;
        link->metrics.jitter_ms = 300U;
        link->metrics.packet_loss_permille = 1000U;
        return;
    }

    link->metrics.measurement_valid = true;
    link->metrics.delay_ms = m->delay_rel_ms;
    link->metrics.jitter_ms = Metrics_GetJitterMs(m);
    link->metrics.packet_loss_permille = Metrics_GetLossPermille(m);

    if (link->metrics.packet_loss_permille >= 150U)
        link->state = LINK_STATE_DEGRADED;
    else
        link->state = LINK_STATE_UP;
}
uint8_t Metrics_IsTimedOut(const rx_metrics_t *m, uint32_t now_ms, uint32_t timeout_ms)
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
}
