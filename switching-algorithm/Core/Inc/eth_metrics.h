#ifndef ETH_METRICS_H
#define ETH_METRICS_H

#include <stdint.h>

#include "eth_rx_types.h"
#include "eth_parser.h"
#include "link_types.h"
typedef int _rx_metrics_guard_t;
void Metrics_Reset(rx_metrics_t *m);

void Metrics_OnFrameReceived(rx_metrics_t *m,
                             const generic_frame_info_t *f,
                             uint32_t rx_tick_ms);

uint32_t Metrics_GetLossPercent(const rx_metrics_t *m);
uint32_t Metrics_GetJitterMs(const rx_metrics_t *m);
uint32_t Metrics_GetThroughputBps(const rx_metrics_t *m);

void Metrics_ApplyToLink(link_info_t *link, const rx_metrics_t *m);
void Metrics_Age(rx_metrics_t *m, uint32_t now_ms, uint32_t timeout_ms);
uint8_t Metrics_IsTimedOut(const rx_metrics_t *m, uint32_t now_ms, uint32_t timeout_ms);
#endif /* ETH_METRICS_H */
