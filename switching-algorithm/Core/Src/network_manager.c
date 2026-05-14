/*
 * network_manager.c
 *
 *  Created on: May 9, 2026
 *      Author: emecc
 */

#include "lwip/err.h"
#include "network_manager.h"
#include "stdio.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "main.h"
#define UDP_PORT 12345

static Interface_t active_if = IF_ETHERNET;
static InterfaceMetrics_t metrics[3]; // Pre ETH, RS485, BT
extern struct udp_pcb *upcb;
extern ip_addr_t DestIPaddr;
void NetMgr_Init(void) {
    active_if = IF_ETHERNET;
    for(int i=0; i<3; i++) {
        metrics[i].health_score = 100;
        metrics[i].packet_loss_pct = 0;
    }
}

// Výpočet zdravia (tá tvoja logika)
static uint8_t Calculate_Health(InterfaceMetrics_t *m) {
    float score = 100.0f;
    score -= (m->packet_loss_pct * 5.0f);
    if (m->rtt_ms > 200) score -= 20;
    if (score < 0) score = 0;
    return (uint8_t)score;
}

void NetMgr_UpdateMetrics(Interface_t iface, float rtt, uint8_t success) {
    if (iface >= IF_NONE) return;

    // Jednoduchý filter pre stratu paketov (klzavý priemer)
    if (!success) {
    	metrics[iface].health_score = 0;
       // metrics[iface].packet_loss_pct = metrics[iface].packet_loss_pct * 0.8f + 20.0f;
    } else {
        metrics[iface].packet_loss_pct = metrics[iface].packet_loss_pct * 0.5f;
        metrics[iface].rtt_ms = rtt;
    }

    metrics[iface].health_score = Calculate_Health(&metrics[iface]);
}

void NetMgr_Decide(void) {
    Interface_t best_if = active_if;

    // Hysteréza: Prepni z ETH na RS485 len ak ETH padne pod 40
    if (active_if == IF_ETHERNET && metrics[IF_ETHERNET].health_score < 40) {

            active_if = IF_RS485;
            printf("\a\n[LOG] Prepnuté na RS485!\n");

    }
    // Návrat na ETH: Len ak je ETH opäť stabilný (nad 80)
    else if (active_if != IF_ETHERNET && metrics[IF_ETHERNET].health_score > 60) {
        active_if = IF_ETHERNET;
        printf("\a\n[LOG] Návrat na ETHERNET!\n");
    }
}

void NetMgr_SendData(uint8_t* data, uint16_t len) {
    switch(active_if) {
        case IF_ETHERNET: {
            if (upcb == NULL) {
                NetMgr_UpdateMetrics(IF_ETHERNET, 0, 0); // Nahlásit chybu, pokud PCB zmizelo
                return;
            }

            // 1. Alokace pbuf
            struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);

            if (p != NULL) {
                // 2. Skopírování dat
                pbuf_take(p, data, len);

                // 3. Odeslání a KONTROLA CHYBY
                if (udp_sendto(upcb, p, &DestIPaddr, UDP_PORT) != ERR_OK) {
                    printf("[ETH] Chyba odoslania!\n");
                    NetMgr_UpdateMetrics(IF_ETHERNET, 0, 0); // Tady se sráží skóre!
                } else {
                    NetMgr_UpdateMetrics(IF_ETHERNET, 20.0f, 1); // Tady se potvrzuje zdraví
                }

                // 4. Uvolnění paměti
                pbuf_free(p);
            } else {
                // Pokud selhala alokace paměti, taky je to problém
                NetMgr_UpdateMetrics(IF_ETHERNET, 0, 0);
            }
            break;
        }

        case IF_RS485:
            printf("[RS485] Posielam: %.*s\n", (int)len, (char*)data);
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
            break;

        default:
            break;
    }
}
uint8_t NetMgr_GetHealth(Interface_t iface) {
    if (iface >= IF_NONE) return 0;
    return metrics[iface].health_score;
}
Interface_t NetMgr_GetActiveInterface(void) {
    return active_if;
}
