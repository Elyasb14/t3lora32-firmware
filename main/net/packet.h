#ifndef PACKET_H
#define PACKET_H

#include "net.h"

typedef struct {
    uint8_t type;
    uint8_t from_id[NODE_ID_LEN];
    uint8_t to_id[NODE_ID_LEN];
    uint32_t seq_num;
    uint8_t public_key[PUBKEY_LEN];
    uint8_t signature[SIG_LEN];
    uint8_t payload[];
} __attribute__((packed)) mesh_packet_t;

#endif // PACKET_H
