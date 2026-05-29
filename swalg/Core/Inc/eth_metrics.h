#ifndef ETH_METRICS_H
#define ETH_METRICS_H

#include <stdint.h>

#include "eth_rx_types.h"
#include "eth_parser.h"
#include "link_types.h"

typedef int _rx_metrics_guard_t;

void     Metrics_Reset(rx_metrics_t *m);

void     Metrics_OnFrameReceived(rx_metrics_t *m,
                                 const generic_frame_info_t *f,
                                 uint32_t rx_tick_ms);

uint32_t Metrics_GetLossPercent(const rx_metrics_t *m);
uint32_t Metrics_GetJitterMs(const rx_metrics_t *m);
uint32_t Metrics_GetThroughputBps(const rx_metrics_t *m);

uint32_t MetricsGetExpectedIntervalMs(const rx_metrics_t* m);
uint32_t MetricsGetTimingErrorMs(const rx_metrics_t* m);
uint32_t MetricsGetMissedIntervals(const rx_metrics_t* m);
uint8_t  MetricsHasExpectedInterval(const rx_metrics_t* m);

/*
 * Metrics_CalcReliability
 * -----------------------
 * Vrati reliability skore 0-1000 (promile; 1000 = idealne).
 *
 * Tri sub-skore, kazde 0-1000:
 *   S1 — timing accuracy : ako blizko prichadzaju ramce k ocakavanemu intervalu
 *                          (ignorovane ak t_expected_ms == 0)
 *   S2 — packet loss     : 1000 - loss_permille
 *   S3 — availability    : podiel prijatych intervalov (bez missed) voci celkovym
 *
 * Vahy w_timing + w_loss + w_avail by mali dat spolu != 0.
 * Odporucane nastavenia:
 *   Periodicke merania (ETH_DIRECT/PLC) : wt=4, wl=4, wa=2
 *   Event-based / BT / RS485           : wt=0, wl=5, wa=5
 */
uint32_t Metrics_CalcReliability(const rx_metrics_t *m,
                                 uint8_t w_timing,
                                 uint8_t w_loss,
                                 uint8_t w_avail);

void     Metrics_ApplyToLink(link_info_t *link, const rx_metrics_t *m);
void     Metrics_Age(rx_metrics_t *m, uint32_t now_ms, uint32_t timeout_ms);
uint8_t  Metrics_IsTimedOut(const rx_metrics_t *m, uint32_t now_ms, uint32_t timeout_ms);

#endif /* ETH_METRICS_H */
