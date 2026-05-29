#ifndef LINK_TYPES_H
#define LINK_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    LINK_NONE = 0,
    LINK_BLUETOOTH,
    LINK_ETH_DIRECT,
    LINK_ETH_PLC,
    LINK_ETH_3,
    LINK_RS485,
    LINK_TYPE_COUNT
} link_type_t;

typedef enum
{
    LINK_STATE_DOWN = 0,
    LINK_STATE_DEGRADED,
    LINK_STATE_UP
} link_state_t;

typedef struct
{
    bool     measurement_valid;
    uint32_t rtt_ms;
    uint32_t jitter_ms;
    uint32_t packet_loss_permille;
    int32_t  signal_dbm;
    bool     security_ok;

    /* Reliability skore 0-1000 (promile); vypocita Metrics_ApplyToLink().
     * 1000 = idealne, 0 = linka je nefunkcna/nespolahliava.
     * Prahove hodnoty pre klasifikaciu:
     *   >= 950  trieda A  (vyborne)
     *   >= 850  trieda B  (akceptovatelne)
     *   >= 700  trieda C  (sledovat)
     *   <  700  trieda F  (zasah potrebny)
     */
    uint32_t reliability_score;
} link_metrics_t;

typedef struct
{
    uint32_t max_loss_percent;
    uint32_t threshold_loss_percent;
    uint32_t threshold_rtt_ms;
    uint32_t threshold_jitter_ms;
} link_thresholds_t;

typedef struct
{
    link_type_t       type;
    bool              enabled;
    link_state_t      state;
    link_metrics_t    metrics;
    link_thresholds_t thresholds;
} link_info_t;

typedef enum
{
    PATH_ETH_DIRECT = 0,
    PATH_ETH_PLC    = 1,
    PATH_BT         = 2,
    PATH_RS485      = 3,
    PATH_COUNT
} path_id_t;

#pragma pack(push, 1)
typedef struct
{
    uint32_t seq;
    uint32_t tx_ts_ms;
    uint8_t  path_id;
    uint8_t  reserved[3];
} test_hdr_t;
#pragma pack(pop)

#endif /* LINK_TYPES_H */
