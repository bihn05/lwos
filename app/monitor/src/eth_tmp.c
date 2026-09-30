#include "eth_tmp.h"

#define REG_CTRL   0x0000
#define REG_STATUS 0x0008
#define REG_ICR    0x00C0
#define REG_IMC    0x00D8
#define REG_RCTL   0x0100
#define REG_RDBAL  0x2800
#define REG_RDBAH  0x2804
#define REG_RDLEN  0x2808
#define REG_RDH    0x2810
#define REG_RDT    0x2818

#define RX_N       8
#define RX_RING    0x00402000
#define RX_BUF     0x00403000

BYTE eth_b, eth_d, eth_f;
DWORD eth_bar0;
DWORD rx_cur;

#define R(off) read_reg(eth_bar0+(off))
#define W(off, v) write_reg(eth_bar0+(off), (v))

void get_eth_bdf(PBYTE b, PBYTE d, PBYTE f) {
    *b=eth_b;
    *d=eth_d;
    *f=eth_f;
}
void set_eth_bdf(BYTE b, BYTE d, BYTE f) {
    eth_b=b;
    eth_d=d;
    eth_f=f;
}
void set_eth_bar(DWORD bar) {
    eth_bar0=bar;
    lw_puts("PRETHERNET: BAR0 VERITIED TO ");
    lw_put_dword(eth_bar0);
    lw_puts("\n\r");
}
static int write_reg(DWORD address, DWORD value) {
    *(PDWORD)(address&0xfffffffc)=value;
    if ((*(PDWORD)(address&0xfffffffc))==value) {
        return 1;
    } else {
        return 0;
    }
}
static DWORD read_reg(DWORD address) {
    return *(PDWORD)(address&0xfffffffc);
}
int e1k_rx_poll(void) {
    unsigned long d = RX_RING + rx_cur * 16;
    unsigned long st = read_reg(d + 12);
    if (!(st & 1))                               /* DD 为 0，没有新帧 */
        return 0;

    //unsigned len = read_reg(d + 8) & 0xFFFF;
    unsigned err = (st >> 8) & 0xFF;
    if (err == 0) {
        lw_dump128((PVOID)(RX_BUF + rx_cur * 2048));
        //on_frame(RX_BUF + rx_cur * 2048, len);
    }

    write_reg(d + 12, 0);                        /* 清状态 */
    W(REG_RDT, rx_cur);                          /* 把这个描述符还给硬件 */
    rx_cur = (rx_cur + 1) % RX_N;
    return 1;
}
void eth_init(void) {
    lw_puts("ETHERNET INIT\n\r");
    lw_puts("DEVICE FOUND AT ");
    lw_puts("BUS ");
    lw_put_byte(eth_b);
    lw_puts(" DEV ");
    lw_put_byte(eth_d);
    lw_puts(" FUNC ");
    lw_put_byte(eth_f);
    lw_puts("\n\r");

    W(REG_IMC,0xffffffff);
    (void)R(REG_ICR);
    W(REG_CTRL, R(REG_CTRL) | 0x40); 

    W(REG_RCTL, 0);
    for (unsigned i = 0; i < RX_N; i++) {
        unsigned long d = RX_RING + i * 16;
        write_reg(d + 0,  RX_BUF + i * 2048);
        write_reg(d + 4,  0);
        write_reg(d + 8,  0);
        write_reg(d + 12, 0);
    }
    W(REG_RDBAL, RX_RING);
    W(REG_RDBAH, 0);
    W(REG_RDLEN, RX_N * 16);
    W(REG_RDH, 0);
    W(REG_RDT, RX_N - 1);
    rx_cur = 0;
    W(REG_RCTL, 0x0400801A);


}