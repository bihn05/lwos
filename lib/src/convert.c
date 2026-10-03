#include "convert.h"
#include "string.h"

BYTE hex_digit(char c) {
    char v=c;
    if ((v&0xf0)==0x30)return v&0xf;
    switch (v&0x60) {
        case 0x40:case 0x60:v+=9;v&=0xf;return v;
        default:return 0xff;
    }
    return 0xff;
}
int hex_parse(PPCSTR p, PDWORD out) {
    PCSTR s = *p;
    DWORD v = 0;
    int digits = 0;
    skip_ws(&s);
    for (;;) {
        unsigned int d = hex_digit(*s);
        if (d==0xff)break;
        v=v<<4;
        v|=(DWORD)(d&0xf);
        digits++;
        s++;
    }
    if (!digits)return 0;
    *p=s;
    *out=v;
    return 1;
}
