#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>

// 替换为你 lwos 打印出来的网卡 MAC 地址
unsigned char target_mac[] = {0x00, 0x26, 0x9e, 0x81, 0x75, 0xb0};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <if_name> <file_path>\n", argv[0]);
        return -1;
    }

    const char *if_name = argv[1];
    const char *file_path = argv[2];

    // 1. 创建原始套接字
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    
    // 2. 获取网卡索引
    struct ifreq ifr;
    strncpy(ifr.ifr_name, if_name, IFNAMSIZ);
    ioctl(sock, SIOCGIFINDEX, &ifr);
    unsigned char src_mac[6];

    // 3. 准备目标地址结构
    struct sockaddr_ll sll = {0};
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_halen = ETH_ALEN;
    memcpy(sll.sll_addr, target_mac, ETH_ALEN);

    // 4. 读取文件
    FILE *f = fopen(file_path, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    rewind(f);
    unsigned char *payload = malloc(fsize);
    fread(payload, 1, fsize, f);
    fclose(f);

    strncpy(ifr.ifr_name, if_name, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
        memcpy(src_mac, ifr.ifr_hwaddr.sa_data, 6);
        printf("Host Src MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4], src_mac[5]);
    }

    // 5. 构造以太网帧 [Dst(6) | Src(6) | Type(2) | Payload(N)]
    unsigned char frame[1514]; 
    memcpy(frame, target_mac, 6);
    // 源 MAC 获取本机 MAC
    memcpy(frame + 6, src_mac, 6);
    // 类型设为自定义实验类型 0x88B5
    frame[12] = 0x88;
    frame[13] = 0xB5;
    
    // 注意：单包限制在 1500 字节以内，如果 ELF 大了需要循环分片发送
    memcpy(frame + 14, payload, fsize > 1480 ? 1480 : fsize);

    // 6. 发送
    sendto(sock, frame, (fsize > 1480 ? 1480 : fsize) + 14, 0, 
           (struct sockaddr*)&sll, sizeof(sll));

    printf("Sent %ld bytes to lwos!\n", fsize);
    return 0;
}