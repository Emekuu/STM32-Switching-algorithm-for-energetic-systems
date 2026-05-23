#include "eth_parser.h"
#include <string.h>

typedef struct __attribute__((packed))
{
    uint32_t seq;
    uint32_t tx_ts_ms;
    uint8_t  path_id;
    uint8_t  reserved[3];
} lab_test_hdr_t;

uint8_t Generic_ParseFrame(const uint8_t *payload,
                           uint16_t len,
                           generic_frame_info_t *out)
{
    if ((payload == NULL) || (out == NULL))
        return 0U;

    memset(out, 0, sizeof(*out));

    if (len < sizeof(lab_test_hdr_t))
        return 0U;

    const lab_test_hdr_t *hdr = (const lab_test_hdr_t *)payload;

    out->seq = hdr->seq;
    out->tx_ts_ms = hdr->tx_ts_ms;
    out->payload_len = len;
    out->path_id = hdr->path_id;
    out->has_tx_ts = 1U;

    return 1U;
}
