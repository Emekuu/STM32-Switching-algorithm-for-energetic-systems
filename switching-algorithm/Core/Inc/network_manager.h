#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "stdint.h"


typedef enum {
    IF_ETHERNET = 0,
    IF_RS485,
    IF_BLUETOOTH,
    IF_NONE
} Interface_t;


typedef struct {
    float rtt_ms;
    float packet_loss_pct;
    uint32_t throughput_bps;
    uint8_t health_score;
} InterfaceMetrics_t;


void NetMgr_Init(void);
void NetMgr_UpdateMetrics(Interface_t iface, float rtt, uint8_t success);
void NetMgr_Decide(void);
void NetMgr_SendData(uint8_t* data, uint16_t len);
uint8_t NetMgr_GetHealth(Interface_t iface);
Interface_t NetMgr_GetActiveInterface(void);

#endif
