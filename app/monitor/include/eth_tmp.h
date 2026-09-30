#ifndef _ETH_TMP_H
#define _ETH_TMP_H

#include "abi.h"

void get_eth_bdf(PBYTE b, PBYTE d, PBYTE f);
void set_eth_bdf(BYTE b, BYTE d, BYTE f);
void set_eth_bar(DWORD bar);

void eth_init(void);
int e1k_rx_poll(void);

#endif