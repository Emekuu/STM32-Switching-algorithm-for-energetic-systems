#include "lwip/udp.h"
#include "udp_echo.h"

#define UDP_ECHO_PORT 5005

static struct udp_pcb *udp_echo_pcb;

static void udp_echo_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  if (p != NULL) {
    udp_sendto(pcb, p, addr, port); // pošle packet naspäť odosielateľovi
    pbuf_free(p);                   // uvoľní packet buffer
  }
}

void udp_echo_init(void)
{
  udp_echo_pcb = udp_new();
  if (udp_echo_pcb != NULL)
  {
    if (udp_bind(udp_echo_pcb, IP_ADDR_ANY, UDP_ECHO_PORT) == ERR_OK)
    {
      udp_recv(udp_echo_pcb, udp_echo_recv, NULL);
    }
    else
    {
      udp_remove(udp_echo_pcb);
      udp_echo_pcb = NULL;
    }
  }
}
