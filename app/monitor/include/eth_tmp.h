#ifndef _ETH_TMP_H
#define _ETH_TMP_H

#include "abi.h"

void get_eth_bdf(PBYTE b, PBYTE d, PBYTE f);
void set_eth_bdf(BYTE b, BYTE d, BYTE f);
void set_eth_bar(DWORD bar);

void eth_init(void);
int e1k_rx_poll(void);

typedef struct _LWFT_HDR {
    DWORD magic;
    BYTE op;
    BYTE flags;
    WORD len;
    DWORD seq;
    DWORD arg;
}  LWFT_HDR, *PLWFT_HDR;

enum {
    OP_HELLO = 1,
    OP_PUT   = 2,
    OP_DATA  = 3,
    OP_END   = 4,
    OP_ACK   = 5,
    OP_NAK   = 6,
    OP_RUN   = 7,
    OP_TEXT  = 8,
};

#endif