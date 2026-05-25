#include "eth_metrics.h"
#include <string.h>

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
	printf("[DBG_RTT] seq=%lu has_ts=%u tx=%lu rx=%lu\n",
	       (unsigned long)f->seq,
	       (unsigned)f->has_tx_ts,
	       (unsigned long)f->tx_ts_ms,
	       (unsigned long)rx_tick_ms);
    if ((m == NULL) || (f == NULL))
        return;
    if (f->has_tx_ts)
    {
        int32_t rtt = (int32_t)rx_tick_ms - (int32_t)f->tx_ts_ms;
        if (rtt < 0)
            rtt = 0;

        m->rtt_ms = (uint32_t)rtt;

        printf("[DBG_RTT2] seq=%lu rtt=%lu\n",
                  (unsigned long)f->seq,
                  (unsigned long)m->rtt_ms);
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
            m->last_tx_ts_ms = f->tx_ts_ms;

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

    if (f->has_tx_ts && (m->last_tx_ts_ms != 0U))
    {
        uint32_t rx_delta = rx_tick_ms - m->last_rx_tick_ms;
        uint32_t tx_delta = f->tx_ts_ms - m->last_tx_ts_ms;

        int32_t variation = (int32_t)rx_delta - (int32_t)tx_delta;
        if (variation < 0)
            variation = -variation;

        Metrics_JitterWindowPush(m, (uint32_t)variation);

        /* RTT ~ reálny interval medzi rámcami na F7 */
        m->rtt_ms = rx_delta;

        m->last_tx_ts_ms = f->tx_ts_ms;
    }
    else
    {
        uint32_t interarrival = rx_tick_ms - m->last_rx_tick_ms;
        Metrics_JitterWindowPush(m, interarrival);
        m->rtt_ms = interarrival;
    }

    m->last_rx_tick_ms = rx_tick_ms;

    /* Ak chceš throughput len z payloadu, nechaj payload_len.
       Ak chceš celý custom frame, použi payload_len + 12U. */
    m->bytes_in_window += f->payload_len;

    if ((rx_tick_ms - m->last_tp_tick_ms) >= 1000U)
    {
        m->throughput_bps = m->bytes_in_window * 8U;
        m->bytes_in_window = 0U;
        m->last_tp_tick_ms = rx_tick_ms;
    }
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

static float App_LossPermilleToPercent(uint32_t loss_permille)
{
    return ((float)loss_permille) / 10.0f;
}
uint32_t Metrics_GetThroughputBps(const rx_metrics_t *m)
{
    if (m == NULL)
        return 0U;

    return m->throughput_bps;
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
        link->metrics.rtt_ms = 1000U;
        link->metrics.jitter_ms = 300U;
        link->metrics.packet_loss_permille = 100U;
        return;
    }

    link->metrics.measurement_valid = true;
    link->metrics.rtt_ms = m->rtt_ms;
    link->metrics.jitter_ms = Metrics_GetJitterMs(m);
    link->metrics.packet_loss_permille = Metrics_GetLossPercent(m);

    if (link->metrics.packet_loss_permille >= 30U)
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
    m->jitter_ms = 0U;

    m->loss_window_sum = m->loss_window_count;
}
