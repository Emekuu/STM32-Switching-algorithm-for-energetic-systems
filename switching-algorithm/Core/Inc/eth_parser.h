#ifndef ETH_PARSER_H
#define ETH_PARSER_H

#include <stdint.h>
#include "eth_rx_types.h"

uint8_t Generic_ParseFrame(const uint8_t *payload,
                           uint16_t len,
                           generic_frame_info_t *out);

#endif
