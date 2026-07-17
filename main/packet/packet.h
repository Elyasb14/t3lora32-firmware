#ifndef PACKET_H 
#define PACKET_H 

#include <stdint.h>

typedef struct {
    char* sender;
    uint8_t* data;
}Packet;

#endif //PACKET_H
