#ifndef ETH_RX_TYPES_H
#define ETH_RX_TYPES_H

#include <stdint.h>

// Destination structure for parsed test data
typedef struct {
    uint32_t seq;           // Packet sequence number
    uint32_t tx_ts_ms;      // Transmission timestamp
    uint16_t payload_len;   // Total length of payload data
    uint8_t  path_id;       // Path identifier
    uint8_t  has_tx_ts;     // Flag: 1 if timestamp is valid, 0 if not
} generic_frame_info_t;

uint8_t Generic_ParseFrame(const uint8_t *buf, uint16_t len, generic_frame_info_t *out);
#endif // ETH_RX_TYPES_H
