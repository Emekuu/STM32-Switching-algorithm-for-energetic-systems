#ifndef INC_COM_INTERFACE_H_
#define INC_COM_INTERFACE_H_

#include <stdint.h>

typedef enum {
    COM_OK = 0,
    COM_ERROR,
    COM_TIMEOUT,
    COM_LINK_DOWN
} ComStatus_t;

// Definícia rozhrania (Interface)
typedef struct {
    ComStatus_t (*Init)(void);                             // Inicializácia (napr. otvorenie socketu)
    ComStatus_t (*Send)(uint8_t* data, uint16_t len);      // Odoslanie dát
    ComStatus_t (*Receive)(uint8_t* buf, uint16_t max_len, uint16_t *received_len); // Príjem dát
    uint8_t     (*IsAlive)(void);                          // Check linky (kabel/bus)
    void        (*Close)(void);                            // Upratanie (zatvorenie socketu)
} ComInterface_t;

#endif /* INC_COM_INTERFACE_H_ */
