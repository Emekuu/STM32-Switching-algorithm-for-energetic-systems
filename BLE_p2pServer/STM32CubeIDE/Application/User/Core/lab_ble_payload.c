#include "lab_ble_payload.h"
#include "stm32wbxx_hal.h"

#define LAB_BLE_PATH_ID   (1U)

static uint32_t s_seq = 0U;

void LabBlePayload_Init(void)
{
    s_seq = 0U;
}

void LabBlePayload_Build(lab_test_hdr_t *hdr)
{
    if (hdr == 0)
    {
        return;
    }

    hdr->seq = s_seq++;
    hdr->tx_ts_ms = HAL_GetTick();
    hdr->path_id = LAB_BLE_PATH_ID;
    hdr->reserved[0] = 0U;
    hdr->reserved[1] = 0U;
    hdr->reserved[2] = 0U;
}
