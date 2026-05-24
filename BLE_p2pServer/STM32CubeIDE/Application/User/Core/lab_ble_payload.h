#ifndef LAB_BLE_PAYLOAD_H
#define LAB_BLE_PAYLOAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct __attribute__((packed))
{
    uint32_t seq;
    uint32_t tx_ts_ms;
    uint8_t  path_id;
    uint8_t  reserved[3];
} lab_test_hdr_t;

void LabBlePayload_Init(void);
void LabBlePayload_Build(lab_test_hdr_t *hdr);

#ifdef __cplusplus
}
#endif

#endif /* LAB_BLE_PAYLOAD_H */
