#ifndef LINK_TYPES_H
#define LINK_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    LINK_BLUETOOTH = 0,
    LINK_ETHERNET,
	LINK_RS485
} link_type_t;

typedef enum
{
    LINK_STATE_DOWN = 0,
    LINK_STATE_DEGRADED,
    LINK_STATE_UP
} link_oper_state_t;

typedef struct
{
    uint32_t rtt_ms;
    uint32_t jitter_ms;
    uint32_t packet_loss_permille;
    int16_t signal_dbm;
    bool security_ok;
    bool measurement_valid;
} link_metrics_t;

typedef struct
{
    link_type_t type;
    bool enabled;
    link_oper_state_t state;
    link_metrics_t metrics;
} link_info_t;

#endif /* LINK_TYPES_H */
