#ifndef PACKET_H
#define PACKET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PACKET_VERSION 1
#define PACKET_MAX_LEN 255 // SX1276 max payload

typedef struct {
    uint8_t version;
    uint8_t name_len;
    uint8_t data_len;
    uint8_t *name;
    uint8_t *data;
} Packet;

// Serialize into buf. Returns bytes written, or 0 if buf is too small.
size_t packet_write_to_buf(Packet *packet, uint8_t *buf);
bool packet_parse(Packet* packet, uint8_t* buf, uint8_t len);

#endif // PACKET_H
