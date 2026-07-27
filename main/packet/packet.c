#include "packet.h"

#include <stdint.h>
#include <string.h>

#define PACKET_MAX_LEN 255

size_t packet_write_to_buf(Packet *packet, uint8_t *buf) {
    buf[0] = packet->version;
    buf[1] = packet->name_len;
    buf[2] = packet->data_len;
    memcpy(buf + 3, packet->name, packet->name_len);
    memcpy(buf + 3 + packet->name_len, packet->data, packet->data_len);

    return 3 + packet->name_len + packet->data_len;
}

Packet packet_parse(uint8_t *buf) {
    uint8_t version = buf[0];
    uint8_t name_len = buf[1];
    uint8_t data_len = buf[2];
    uint8_t *name = buf + 3;
    uint8_t *data = name + name_len;

    Packet packet = {.version = version, .name_len = name_len, .data_len = data_len, .name = name, .data = data};

    return packet;
}
