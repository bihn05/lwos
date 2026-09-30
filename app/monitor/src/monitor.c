
#include "abi.h"
#include "ctx.h"

#include "eth.h"

PVOID *lw_abi_base;

/* ABI 不可用时连 puts 都没有, 只能直接写显存 */
static void no_abi_halt(void) {
    const char *msg = "NO ABI.BIN AT 100000H";
    PWORD video = (PWORD)0xB8000;
    for (int i = 0; msg[i]; i++) {
        video[i] = 0x0c00 | (BYTE)msg[i];
    }
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

static int fb_w, fb_h, fb_pitch;
static PDWORD framebuffer;
static char line[128];

void putpixel(int x, int y, DWORD color) {
    if((x>=fb_w)||(x<0)||(y>=fb_h)||(y<0))return;
    framebuffer[x+y*fb_pitch]=color&0xffffff;
}

static void prompt_read() {
    int n = 0;
    lw_puts(">");
    for (;;) {
        char c = lw_getc();
        if (c=='\n') { line[n]=0; lw_puts("\n\r"); return; }
        if (c=='\b') { if (n>0) { n--; lw_putc('\b'); } continue; }
        if (c<' '||c>'~'||n>=(int)sizeof(line)-1)
            continue;
        line[n++]=c;
        lw_putc(c);
    }
}

static void skip_ws(const char **p) {
    while (**p==' '||**p=='\t') {
        (*p)++;
    }
}

static int hex_catch(char c) {
    switch (c) {
        case '0':return 0x0;
        case '1':return 0x1;
        case '2':return 0x2;
        case '3':return 0x3;
        case '4':return 0x4;
        case '5':return 0x5;
        case '6':return 0x6;
        case '7':return 0x7;
        case '8':return 0x8;
        case '9':return 0x9;
        case 'a':case 'A':return 0xa;
        case 'b':case 'B':return 0xb;
        case 'c':case 'C':return 0xc;
        case 'd':case 'D':return 0xd;
        case 'e':case 'E':return 0xe;
        case 'f':case 'F':return 0xf;
        default:return -1u;
    }
}

extern void pci_scan(void);
extern void pci_detail(BYTE bus,BYTE dev,BYTE fn);
extern int pci_find_class(BYTE cls, BYTE sub, PBYTE bus, PBYTE dev, PBYTE fn);

static int parse_hex(const char **p, PDWORD out) {
    const char *s = *p;
    DWORD v = 0;
    int digits = 0;

    skip_ws(&s);
    for (;;) {
        unsigned int d = hex_catch(*s);
        if (d==-1u)break;
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

static void execute(const char* str) {
    char c0;
    char c1;
    skip_ws(&str);
    DWORD a;
    DWORD b;

    if (!*str)return;
    c0 = *str++;
	c1 = (*str&&*str!=' '&&*str!='\t')?
            *str++:0;

    switch (c0) {
        case 'h': {
            lw_puts("d [ADDR]\n\r");
            break;
        }
        case 'd': {
            if (!parse_hex(&str, &a)) {
                return;
            }
            lw_dump128((PVOID)a);
            break;
        }
        case 'o': { // out/write
            if (!parse_hex(&str, &a)) {
                return;
            }
            if (!parse_hex(&str,&b)) {
                return;
            }
            switch (c1) {
                case 'b': {
                    a&=0xffffffff;
                    lw_put_dword(a);
                    lw_puts(":");
                    lw_put_byte(b);
                    lw_puts("\n\r");
                    *(PBYTE)(a)=(BYTE)b;
                    if (*(PBYTE)a!=(BYTE)b) {
                        lw_puts("WROTE FAIL\n\r");
                        lw_put_dword(a);
                        lw_puts(":");
                        lw_put_byte(*(PBYTE)a);
                        lw_puts("\n\r");
                    }
                    break;
                }
                case 'w': {
                    a&=0xfffffffe;
                    lw_put_dword(a);
                    lw_puts(":");
                    lw_put_word(b);
                    lw_puts("\n\r");
                    *(PWORD)(a)=(WORD)b;
                    if (*(PWORD)a!=(WORD)b) {
                        lw_puts("WROTE FAIL\n\r");
                        lw_put_dword(a);
                        lw_puts(":");
                        lw_put_word(*(PWORD)a);
                        lw_puts("\n\r");
                    }
                    break;
                }
                case 'l': {
                    a&=0xfffffffc;
                    lw_put_dword(a);
                    lw_puts(":");
                    lw_put_dword(b);
                    lw_puts("\n\r");
                    *(PDWORD)(a)=(DWORD)b;
                    if (*(PDWORD)a!=(DWORD)b) {
                        lw_puts("WROTE FAIL\n\r");
                        lw_put_dword(a);
                        lw_puts(":");
                        lw_put_byte(*(PDWORD)a);
                        lw_puts("\n\r");
                    }
                    break;
                }
                default:return;
            }
            break;
        }
        case 'i': { // in/read
            DWORD d;
            if (!parse_hex(&str, &a)) {
                return;
            }
            switch (c1) {
                case 'b': {
                    a&=0xffffffff;
                    d=*(PBYTE)a;
                    lw_put_dword(a);
                    lw_puts(":");
                    lw_put_byte(d);
                    lw_puts("\n\r");
                    break;
                }
                case 'w': {
                    a&=0xfffffffe;
                    d=*(PWORD)a;
                    lw_put_dword(a);
                    lw_puts(":");
                    lw_put_word(d);
                    lw_puts("\n\r");
                    break;
                }
                case 'l': {
                    a&=0xfffffffc;
                    d=*(PDWORD)a;
                    lw_put_dword(a);
                    lw_puts(":");
                    lw_put_dword(d);
                    lw_puts("\n\r");
                    break;
                }
                default:return;
            }
            break;
        }
        case 'n': {
            if (c1==0) {
                if (pci_find_class(2,0,&eth_b,&eth_d,&eth_f)==0) {
                    lw_puts("NO ETHERNET DEVICE FOUND\n\r");
                    return;
                }
                
                return;
            }
            break;
        }
        case 'p': {
            DWORD bus, dev, fn;
            if (!parse_hex(&str,&bus)) {
                pci_scan();
                return;
            }
            if (!parse_hex(&str,&dev) || !parse_hex(&str, &fn)) {
                pci_scan();
                return;
            }
            if (bus > 0xFF || dev > 31 || fn > 7) {
                lw_puts("INVALID BDF\n\r");
                return;
            }
            pci_detail((BYTE)bus,(BYTE)dev,(BYTE)fn);
            break;
        }
        case 'v': {
            int rc;
            if (c1==0) {
                lw_puts("GFX ");
                lw_puts(lw_gfx_is_live()?"LIVE":"OFF ");
                lw_puts("\n\r");
                lw_puts("FRAMEBUF=");
                lw_put_dword((DWORD)lw_get_fb());
                lw_puts(" ");
                lw_put_word((DWORD)lw_get_fb_w());
                lw_puts("x");
                lw_put_word((DWORD)lw_get_fb_h());
                lw_puts(" ");
                lw_put_byte((DWORD)lw_get_fb_bpp());
                lw_puts("BIT PITCH=");
                lw_put_dword((DWORD)lw_get_fb_pitch()/4);
                lw_puts("*DWORD\n\r");
            } else if (c1=='e') {
                rc=lw_gfx_enter();
                if (rc) {
                    lw_puts(rc==-1?"NO USABLE MODE IN BOOT INFO\n\r":
                            "ROM REFUSED THE MODE\n\r");
                    return;
                }
            } else if (c1=='x') {
                lw_gfx_exit();
            }
            break;
        }
        case '.': {
            for (int i=0;i<fb_h;i++) {
                for (int j=0;j<fb_w;j++) {
                    framebuffer[j+i*fb_w]=0;
                }
            }

            for (int i=0;i<fb_w;i++) {
                putpixel(i, fb_h/2, 0xffffff);
            }
            for (int i=0;i<fb_h;i++) {
                putpixel(fb_w/2, i, 0xffffff);
            }

            break;
        }
    }
}

__attribute__((section(".text.start")))
void monitor_main(void) {

    if (lw_abi_attach((PVOID)LW_ABI_BASE) != 0) {
        no_abi_halt();
    }
    lw_puts("\n\rLWOS MONITOR v2 COPYLEFT 2026\n\r");

    idt_init();
    lw_kbd_probe();
    lw_kbd_enable();
    lw_fpu_init();

    fb_h = lw_get_fb_h();
    fb_w = lw_get_fb_w();
    fb_pitch = lw_get_fb_pitch()/4;
    framebuffer = (PDWORD)lw_get_fb();

    lw_resv_entry();

    while (1) {
        prompt_read();
        execute(line);
    }
}
