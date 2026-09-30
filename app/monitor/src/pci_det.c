// pci detect

#include "abi.h"

const char *pci_class_name(BYTE cls, BYTE sub)
{
	switch (cls) {
	case 0x00: return "unclassified\0";
	case 0x01:
		switch (sub) {
		case 0x01: return "storage IDE\0";
		case 0x06: return "storage SATA/AHCI\0";
		case 0x08: return "storage NVMe\0";
		}
		return "storage\0";
	case 0x02:
		return sub == 0x00 ? "network ethernet\0" :
		       sub == 0x80 ? "network other\0" : "network\0";
	case 0x03:
		return sub == 0x00 ? "display VGA\0" : "display\0";
	case 0x04:
		switch (sub) {
		case 0x00: return "audio (legacy AC97)\0";
		case 0x01: return "audio (MIDI)\0";
		case 0x03: return "audio HDA\0";
		}
		return "multimedia";
	case 0x05: return "memory controller\0";
	case 0x06:
		switch (sub) {
		case 0x00: return "bridge host\0";
		case 0x01: return "bridge ISA\0";
		case 0x04: return "bridge PCI-to-PCI\0";
		}
		return "bridge\0";
	case 0x07: return "serial/comm\0";
	case 0x08: return "system peripheral\0";
	case 0x09: return "input\0";
	case 0x0B: return "cpu\0";
	case 0x0C:
		switch (sub) {
		case 0x03: return "usb\0";
		case 0x05: return "smbus\0";
		}
		return "serial bus\0";
	case 0x0D: return "wireless\0";
	case 0x11: return "signal processing\0";
	}
	return "device\0";
}

const char *pci_vendor_name(WORD ven)
{
	switch (ven) {
	case 0x8086: return "Intel\0";
	case 0x1002: return "AMD/ATI\0";
	case 0x1022: return "AMD\0";
	case 0x10DE: return "NVIDIA\0";
	case 0x1234: return "QEMU\0";
	case 0x1AF4: return "virtio\0";
	case 0x1B36: return "Red Hat\0";
	case 0x10EC: return "Realtek\0";
	case 0x14E4: return "Broadcom\0";
	case 0x1969: return "Atheros\0";
	case 0x1106: return "VIA\0";
	case 0x5333: return "S3\0";
	case 0x80EE: return "VirtualBox\0";
	case 0x15AD: return "VMware\0";
	}
	return 0;
}
static const char *cap_name(BYTE id)
{
	switch (id) {
	case 0x01: return "power management\0";
	case 0x02: return "AGP\0";
	case 0x03: return "vital product data\0";
	case 0x05: return "MSI\0";
	case 0x09: return "vendor specific\0";
	case 0x0A: return "debug port\0";
	case 0x10: return "PCI Express\0";
	case 0x11: return "MSI-X\0";
	case 0x12: return "SATA config\0";
	}
	return "capability\0";
}
void print_bdf(BYTE bus,BYTE dev,BYTE fn) {
    lw_put_byte(bus);
    lw_puts(":");
    lw_put_byte(dev);
    lw_puts(":");
    lw_putc((char)(0x30+fn));
}

void print_id(WORD ven, WORD devid) {
    const char *vn=pci_vendor_name(ven);

    lw_put_word(ven);
    lw_puts(":");
    lw_put_word(devid);
    if (vn) {
        lw_putc(' ');
        lw_puts((char*)vn);
    }
}

void pci_scan(void) {
    DWORD found = 0;

    if (!lw_pci_present()) {
        lw_puts("NO PCI HOST BRIDGE FOUND\n\r");
        return;
    }

    lw_puts("PCI : \n\r");
    lw_puts("bfd      class               vendor:device      irq\n\r");

    for (DWORD bus=0;bus<256;bus++) {
        for (DWORD dev=0;dev<32;dev++) {
            BYTE nfn=1;

            for (BYTE fn=0;fn<nfn;fn++) {
                // VENDOR ID -> +0:0000ffffh
                WORD ven=lw_pci_read((BYTE)bus,dev,fn,0)&0xffff;
                BYTE cls, sub, irq, hdr;
                const char* name;

                if (ven==0xffff||ven==0) {
                    continue;
                }
                if (fn==0) {
                    // HEADER TYPE -> +c:00ff0000h
                    hdr=(lw_pci_read((BYTE)bus,dev,fn,0xc)>>16)&0xff;
                    if (hdr&0x80)nfn=8;
                }
                // CLASS -> +8:ff000000h
                cls=(lw_pci_read((BYTE)bus,dev,fn,8)>>24)&0xff;
                // SUBCLASS -> +8:00ff0000h
                sub=(lw_pci_read((BYTE)bus,dev,fn,8)>>16)&0xff;
                // IRQLINE -> +3c:000000ffh
                irq=(lw_pci_read((BYTE)bus,dev,fn,0x3c)&0xff);
                name=pci_class_name(cls,sub);

                print_bdf((BYTE)bus,dev,fn);
                lw_puts("  ");
                lw_puts_pad(name, 20);
                // DEVICE ID -> +0:ffff0000h
                print_id(ven,(lw_pci_read(
                    (BYTE)bus,dev,fn,0)>>16)&0xffff
                );
                if (irq!=0xff&&irq!=0) {
                    lw_puts("   ");
                    lw_put_byte(irq);
                }
                lw_puts("\n\r");
                found++;
            }
        }
    }

    lw_puts("FUNCTION FOUND: ");
    lw_put_dword(found);
    lw_puts("\n\r");
}
static DWORD bar_size(BYTE bus, BYTE dev, BYTE fn, BYTE off, DWORD orig, int io)
{
	DWORD mask;

	lw_pci_write(bus, dev, fn, off, 0xFFFFFFFFu);
	mask = lw_pci_read(bus, dev, fn, off);
	lw_pci_write(bus, dev, fn, off, orig);

	/* An IO BAR only implements the low 16 bits; the high half reads back 0,
	 * which inverts to 1 and makes a 8-byte region print as 0xFFFF0008. The
	 * address side already truncates with a (u16) cast, the size side did not.
	 * Narrow to 16 bits before inverting, and mask bits 1:0 (bit0 is the
	 * space indicator, bit1 reserved). */
	if (io) {
		mask &= 0xFFFCu;
		if (!mask)
			return 0;
		return (DWORD)((WORD)(~mask) + 1u);
	}

	mask &= 0xFFFFFFF0u;
	if (!mask)
		return 0;
	return (~mask) + 1;
}
static void print_size(DWORD v) {
	static const char unit[] = { 'B', 'K', 'M', 'G' };
	int u = 0;

	while (u < 3 && v >= 1024 && (v & 1023) == 0) {
		v >>= 10;
		u++;
	}
	lw_put_dword(v);
    lw_puts("H ");
	lw_putc(unit[u]);
}
static void dump_bars(BYTE bus, BYTE dev, BYTE fn)
{
	for (int i = 0; i < 6; i++) {
		BYTE off = (BYTE)(0x10 + i * 4);
		DWORD v   = lw_pci_read(bus, dev, fn, off);
		DWORD sz;
		int io;

		if (!v)
			continue;

		io = v & 1;
		lw_puts("  bar");
		lw_putc((char)('0' + i));
		lw_puts(io ? "  io   " : "  mem  ");

		sz = bar_size(bus, dev, fn, off, v, io);

		if (io) {
			lw_put_word((WORD)(v & 0xFFFFFFFCu));
		} else {
			int is64 = ((v >> 1) & 3) == 2;

			lw_put_dword(v & 0xFFFFFFF0u);
			lw_puts(is64 ? " 64-bit" : " 32-bit");
			if (v & 8)
				lw_puts(" prefetchable");
			if (is64) {
				/* the next slot is the high half, not a BAR */
				DWORD hi = lw_pci_read(bus, dev, fn, (BYTE)(off + 4));
				if (hi) {
					lw_puts(" high ");
					lw_put_dword(hi);
				}
				i++;
			}
		}
		if (sz) {
			lw_puts("  size ");
			print_size(sz);
		}
		lw_puts("\n\r");
	}
}
static void dump_caps(BYTE bus, BYTE dev, BYTE fn)
{
	WORD status = (lw_pci_read(bus, dev, fn, 4)>>16)&0xffff;
	BYTE ptr;

	if (!(status & 0x10))           /* capability list bit */
		return;

	ptr = lw_pci_read(bus, dev, fn, 0x34) & 0xFC;
	/* Bounded: a malformed or circular list must not spin forever. */
	for (int guard = 0; ptr && ptr != 0xFF && guard < 48; guard++) {
		BYTE id   = lw_pci_read(bus, dev, fn, ptr)&0xff;
		BYTE next = (lw_pci_read(bus, dev, fn, ptr)>>8)&0xff;

		lw_puts("  cap ");
		lw_put_byte(ptr);
		lw_puts("  ");
		lw_put_byte(id);
		lw_puts(" ");
		lw_puts(cap_name(id));
		lw_puts("\n\r");
		ptr = next & 0xFC;
	}
}
int pci_find_class(BYTE cls, BYTE sub, PBYTE bus, PBYTE dev, PBYTE fn) {
    if (!lw_pci_present()) {
        return 0;
    }

    for (DWORD b=0;b<256;b++) {
        for (BYTE d=0;d<32;d++) {
            BYTE nfn=1;

            for (BYTE f=0;f<nfn;f++) {
                WORD ven=lw_pci_read((BYTE)b,d,f,0)&0xffff;
                BYTE hdr;

                if (ven==0xffff||ven==0) {
                    continue;
                }

                if (f==0) {
                    hdr=(lw_pci_read((BYTE)b,d,0,0xc)>>8)&0xff;
                    if (hdr&0x80) {
                        nfn=8;
                    }
                }

                if ((((lw_pci_read((BYTE)b,d,f,8)>>24)&0xff)==cls)&&
                (((lw_pci_read((BYTE)b,d,f,8)>>16)&0xff)==sub)) {
                    *bus=(BYTE)b;
                    *dev=d;
                    *fn=f;
                    return 1;
                }
            }
        }
    }
    return 0;
}
void pci_detail(BYTE bus, BYTE dev, BYTE fn) {
    WORD ven=lw_pci_read(bus,dev,fn,0)&0xffff;
    BYTE cls,sub,pif,hdr,irq,pin;

    if (ven==0xffff||ven==0) {
        lw_puts("NOTHING AT ");
        print_bdf(bus,dev,fn);
        lw_puts("\n\r");
        return;
    }

    cls=(lw_pci_read(bus,dev,fn,0x8)>>24)&0xff;
    sub=(lw_pci_read(bus,dev,fn,0x8)>>16)&0xff;
    pif=(lw_pci_read(bus,dev,fn,0x8)>>8)&0xff;
    hdr=(lw_pci_read(bus,dev,fn,0xc)>>16)&0xff;
    irq=(lw_pci_read(bus,dev,fn,0x3c))&0xff;
    pin=(lw_pci_read(bus,dev,fn,0x3c)>>8)&0xff;

    print_bdf(bus,dev,fn);
    lw_puts("  ");
    print_id(ven,(lw_pci_read(bus,dev,fn,0)>>16)&0xffff);
    lw_puts("\n\r");

    lw_puts("  class    ");
    lw_put_byte(cls); lw_puts(" "); lw_put_byte(sub); lw_puts(" "); lw_put_byte(pif);
    lw_puts("  "); lw_puts(pci_class_name(cls, sub)); lw_puts("\n\r");

    lw_puts("  subsys   ");
    lw_put_word(lw_pci_read(bus,dev,fn,0x2c)&0xffff);
    lw_puts(":");
    lw_put_word((lw_pci_read(bus,dev,fn,0x2c)>>16)&0xffff);
    lw_puts("  rev ");
    lw_put_byte(lw_pci_read(bus,dev,fn,0x8)&0xff);
    lw_puts("\n\r");

    lw_puts("  commmand ");
    lw_put_word(lw_pci_read(bus,dev,fn,4)&0xffff);
    lw_puts("  status ");
    lw_put_word((lw_pci_read(bus,dev,fn,4)>>16)&0xffff);
    lw_puts("  hdr ");
    lw_put_byte(hdr);
    if (hdr&0x80) {
        lw_puts("MULTIFUNC");
    }
    lw_puts("\n\r");

    if (pin) {
        lw_puts("  irq      pin ");
        lw_putc((char)('A'+pin-1));
        lw_puts("  line ");
        if (irq==0xff) {
            lw_puts("not routed");
        } else {
            lw_put_dword(irq);
        }
        lw_puts("\n\r");
    }

    dump_bars(bus,dev,fn);
    dump_caps(bus,dev,fn);
}