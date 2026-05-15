/*
 * drv_eth.h
 *
 *  Created on: May 15, 2026
 *      Author: emecc
 */

#ifndef SRC_DRV_ETH_H_
#define SRC_DRV_ETH_H_



#include <stdbool.h>
#include "d_decision.h"

typedef enum
{
    ETH_SEC_DISCONNECTED = 0,
    ETH_SEC_TCP_CONNECTED,
    ETH_SEC_TLS_HANDSHAKING,
    ETH_SEC_TLS_ESTABLISHED,
    ETH_SEC_ERROR
} eth_security_state_t;

void ethernet_security_init(link_info_t *eth_link);
void ethernet_security_on_tcp_connected(link_info_t *eth_link);
void ethernet_security_on_tls_handshake_ok(link_info_t *eth_link, bool peer_verified);
void ethernet_security_on_tls_error(link_info_t *eth_link);
void ethernet_security_on_disconnect(link_info_t *eth_link);

#endif
