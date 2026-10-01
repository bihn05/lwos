#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define LW_ETHERTYPE    0x88B5
#define LWFT_MAGIC      0x5446574Cu     /* 'LWFT' 小端 */
#define LWFT_MAX_DATA   1024

enum {
    OP_HELLO = 1,
    OP_PUT   = 2,
    OP_DATA  = 3,
    OP_END   = 4,
    OP_ACK   = 5,
    OP_NAK   = 6,
    OP_RUN   = 7,
    OP_TEXT  = 8,       /* 控制台文本, 当串口用 */
};

struct lwft_hdr {
    uint32_t magic;
    uint8_t  op;
    uint8_t  flags;
    uint16_t len;       /* 后面 data 的字节数 */
    uint32_t seq;
    uint32_t arg;
} __attribute__((packed));

_Static_assert(sizeof(struct lwft_hdr) == 16, "lwft_hdr");

static int sock=-1;
static int ifindex;
static uint8_t src_mac[6];
static uint8_t dst_mac[6]={0xff,0xff,0xff,0xff,0xff,0xff};
static int raw_mode;
static uint32_t tx_seq;

static void net_open(const char *ifname) {
    struct ifreq ifr;

    // AF_PACKET == LAYER 2
    // SOCK_RAW  == RAW FRAME FROM ETHER
    sock=socket(AF_PACKET,SOCK_RAW,htons(0x88b5));
    if (sock<0) {
        if (errno==EPERM) {
            fprintf(stderr,"root needed\n");
        }
        exit(1);
    }

    memset(&ifr,0,sizeof ifr);
    snprintf(
        ifr.ifr_ifrn.ifrn_name,
        IFNAMSIZ,
        "%s", ifname
    );
    if (ioctl(sock, SIOCGIFINDEX, &ifr)<0) {
        fprintf(stderr, "SIOCGIFINDEX");
        exit(1);
    }
    ifindex=ifr.ifr_ifru.ifru_ivalue;
    if (ioctl(sock, SIOCGIFHWADDR, &ifr)<0) {
        fprintf(stderr, "SIOCGIFHWADDR");
        exit(1);
    }
    memcpy(
        src_mac,
        ifr.ifr_ifru.ifru_hwaddr.sa_data,
        6
    );

    struct sockaddr_ll sll = {
        .sll_family = AF_PACKET,
        .sll_protocol = htons(0x88b5),
        .sll_ifindex = ifindex,
    };
    if (bind(sock, (struct sockaddr *)&sll,sizeof sll)<0) {
        fprintf(stderr, "bind");
        exit(1);
    }
}

static int net_send(
    uint8_t op,
    uint32_t arg,
    const void *data,
    size_t len
) {
    uint8_t frame[ETH_FRAME_LEN];
    size_t n=0;
    if (len>1024)len=1024;

    memcpy(frame+0,dst_mac,6);
    memcpy(frame+6,src_mac,6);
    frame[12]=0x88;
    frame[13]=0xb5;
    n=14;
    
    struct lwft_hdr h=
    {
        .magic=LWFT_MAGIC,
        .op=op,
        .flags=0,
        .len=(uint16_t)len,
        .seq=tx_seq++,
        .arg=arg,
    };

    memcpy(frame+n,&h,sizeof h);
    n+=sizeof h;

    memcpy(frame+n,data,len);
    n+=len;

    if (n<60) {
        memset(frame+n,0,60-n);
        n=60;
    }

    struct sockaddr_ll to = {
        .sll_family=AF_PACKET,
        .sll_ifindex=ifindex,
        .sll_halen=6,
    };
    memcpy(to.sll_addr,dst_mac,6);


    if (sendto(sock,frame,n,0,(struct sockaddr *)&to,sizeof to)<0) {
        fprintf(stderr, "sendto");
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    const char *fixed=NULL;
    if (argc<2||argv[1][0]=='-') {
        printf("syntax error\n");
        return 0;
    }
    const char *ifname=argv[1];
    int opt;
    while ((opt=getopt(argc,argv,"s:"))!=-1) {
        switch (opt) {
            case 's': {
                fixed=optarg;
                break;
            }
        }
    }

    printf("net open %s\n", ifname);
    net_open(ifname);

    if (fixed) {
        net_send(OP_RUN,0,fixed,strlen(fixed));
    } else {
        printf("NO CONTENT\n");
    }

    close(sock);

    return 0;
}