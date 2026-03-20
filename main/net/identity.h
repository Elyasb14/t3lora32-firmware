#ifndef IDENTITY_H
#define IDENTITY_H

#include "net.h"
#include "esp_err.h"

typedef struct {
    uint8_t public_key[PUBKEY_LEN];
    uint8_t private_key[PRIVKEY_LEN];
    uint8_t node_id[NODE_ID_LEN];
} node_identity_t;

esp_err_t identity_init(node_identity_t *id);
esp_err_t identity_sign(const node_identity_t *id, const uint8_t *msg,
                       size_t msg_len, uint8_t *signature);
esp_err_t identity_verify(const uint8_t *public_key,
                          const uint8_t *msg, size_t msg_len,
                          const uint8_t *signature);

#endif // IDENTITY_H
