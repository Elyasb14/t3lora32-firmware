#ifndef NET_H
#define NET_H

#include <stdint.h>

#define NODE_ID_LEN    8
#define PUBKEY_LEN    64
#define PRIVKEY_LEN   32
#define SIG_LEN       64

#define PKT_TYPE_TEXT       0x01
#define PKT_TYPE_ACK        0x02
#define PKT_TYPE_ROUTE_REQ  0x03
#define PKT_TYPE_ROUTE_RSP  0x04

#endif // NET_H
