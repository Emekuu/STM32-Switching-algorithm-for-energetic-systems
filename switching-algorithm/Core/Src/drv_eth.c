#include "com_int.h"
#include "lwip/sockets.h"
#include <string.h>
#include <stdio.h>
#include "d_decision.h"
static link_info_t *eth_link_info = NULL;

extern ETH_HandleTypeDef heth;
extern struct netif gnetif;

static int sock = -1;
static struct sockaddr_in servaddr;



static ComStatus_t Eth_Init(void) {
    if (sock >= 0) lwip_close(sock);

    sock = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) return COM_ERROR;


    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(12345);
    servaddr.sin_addr.s_addr = inet_addr("192.168.0.131");

    Eth_UpdateSecurityStatus();

    return COM_OK;
}

static void Eth_Close(void) {
    if (sock >= 0) {
        lwip_close(sock);
        sock = -1;
    }

    if (eth_link_info != NULL) {
        eth_link_info->metrics.security_ok = false;
    }
}

static uint8_t Eth_IsAlive(void) {
    uint32_t phy_reg = 0;

    HAL_ETH_ReadPHYRegister(&heth, 1, 1, &phy_reg);

    uint8_t link = (phy_reg & 0x0004) ? 1 : 0;

    if (!link && sock >= 0) {
        Eth_Close();
        netif_set_link_down(&gnetif);
    }

    if (eth_link_info != NULL) {
        if (!link) {
            eth_link_info->metrics.security_ok = false;
        } else {
            Eth_UpdateSecurityStatus();
        }
    }

    return link;
}

static ComStatus_t Eth_Send(uint8_t* data, uint16_t len) {
    if (sock < 0) return COM_ERROR;
    int res = lwip_sendto(sock, data, len, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    return (res > 0) ? COM_OK : COM_ERROR;
}

static ComStatus_t Eth_Receive(uint8_t* buf, uint16_t max_len, uint16_t *received_len) {
    if (sock < 0) return COM_ERROR;

    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);


    int n = lwip_recvfrom(sock, buf, max_len, MSG_DONTWAIT, (struct sockaddr *)&from, &fromlen);

    if (n > 0) {
        *received_len = (uint16_t)n;
        return COM_OK;
    }
    return COM_TIMEOUT;
}


void Eth_RegisterLinkInfo(link_info_t *link)
{
    eth_link_info = link;
}

static void Eth_UpdateSecurityStatus(void)
{
    if (eth_link_info == NULL)
        return;

    /* actual implementation uses UDP without DTLS, so its not secure. */
    eth_link_info->metrics.security_ok = false;
}

ComInterface_t EthernetInterface = {
    .Init = Eth_Init,
    .Send = Eth_Send,
    .Receive = Eth_Receive,
    .IsAlive = Eth_IsAlive,
    .Close = Eth_Close
};





