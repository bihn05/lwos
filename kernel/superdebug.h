#ifndef _SUPER_DEBUGGING_H
#define _SUPER_DEBUGGING_H

#include <conio.h>
#include <kernel.h>
#include <stdint.h>
#include <mem.h>
#include <driver/acpi.h>
#include <driver/ata.h>
#include <service/str_to_num.h>

#define S_PROMPT_BUFFER_MAX 512
char sp_buffer[S_PROMPT_BUFFER_MAX];
uint16_t cmd;
void getlinek(const char* ds);
void s_get_command(void);
void s_execute(uint16_t command);
void super_dbg(void);
void cmd_dmp(void);
void s_edhd(void);

void getlinek(const char* ds) {
	int count = 0;
	bool block = true;
	char buf[S_PROMPT_BUFFER_MAX];
	char c;
	while (block) {
		c = getch();
		switch (c) {
		case 0: {
			break;
		}
		case '\n': {
			buf[count] = 0;
			block = false;
			printk("\n");
			break;
		}
		case '\b': {
			count--;
			if (count < 0) {
				count = 0;
			}
			else {
				printk("\b \b");
			}
			break;
		}
		default: {
			buf[count++] = c;
			putchar(c);
			break;
		}
		}
		c = 0;
	}
	strcpy(ds, buf);
}
void s_get_command() {
	memset(sp_buffer, 0, S_PROMPT_BUFFER_MAX);
	printk(" $ ");
	getlinek(sp_buffer);
	if (strlen(sp_buffer) == 0)cmd = 0;
	if (strcmp(sp_buffer, "help") == 0)cmd = 1;
	if (strcmp(sp_buffer, "stdn") == 0)cmd = 2;
	if (strcmp(sp_buffer, "miao") == 0)cmd = 3;
	if (strcmp(sp_buffer, "meow") == 0)cmd = 4;
	if (strcmp(sp_buffer, "dmp") == 0)cmd = 5;
	if (strcmp(sp_buffer, "edhd") == 0)cmd = 6;
	if (strcmp(sp_buffer, "cls") == 0)cmd = 7;

	s_execute(cmd);
	cmd = 0xffff;
}
void s_execute(uint16_t command) {
//	printk("strlen=%d cmd=%d\n", strlen(sp_buffer), cmd);
//	printk("str=%s\n", sp_buffer);
	switch (command) {
	case 0: {
		break;
	}
	case 1: {
		printk("cd   - *change current directory\n");
		printk("cls  - clear screen buffer\n");
		printk("cp   - *copy entry\n");
		printk("dmp  - dump memory\n");
		printk("edhd - edit hard drive\n");
		printk("edmm - *edit memory\n");
		printk("help - list all command\n");
		printk("ls   - *list all entry under current directory\n");
		printk("md   - *make a new directory\n");
		printk("mv   - *move entry\n");
		printk("rd   - *remove directory\n");
		printk("rm   - *remove entry\n");
		printk("rn   - *rename entry\n");
		printk("stdn - shutdown the system\n");
		break;
	}
	case 2: {
		printk("now you can power off safely\n");
		acpi_shutdown();
		break;
	}
	case 3: {
		printk("owO\n");
		break;
	}
	case 4: {
		printk("Owo\n");
		break;
	}
	case 5: {
		cmd_dmp();
		break;
	}
	case 6: {
		s_edhd();
		break;
	}
	case 7: {
		clear_device();
		break;
	}
	case 0xffff: {
		printk("Invaild command.\n");
		break;
	}
	}
}
void super_dbg(void) {
	while (1) {
		s_get_command();
	}
}
void cmd_dmp(void) {
	static char buf1[9];
	uint32_t addr = 0;
	uint32_t cnt = 0;
	printk("address(32bit, hex)=");
	getlinek(buf1);
	addr = hstr_to_uint32(buf1);
	printk("count(*256 bytes)=");
	getlinek(buf1);
	cnt = str_to_num(buf1);
	dump_chunk(addr, cnt);
}
const char hex[17] = "0123456789ABCDEF";
int s_edit_sector = 0;
int is_upper = 0;
int s_offset = 0;
int c_offset = 0;
uint8_t s_e_buffer[512];
void out_char(uint8_t ch, int c) {
	char buf[3];
	buf[0] = hex[ch / 0x10 % 0x10];
	buf[1] = hex[ch % 0x10];
	buf[2] = 0;
	putchar_c(buf[0], c);
	putchar_c(buf[1], c);
}
void s_edhd(void) {
	char c;
	while (1) {
		clear_device();
		set_cursor(20, 0);
		printk("%d/%d sector", s_edit_sector, ata_devices->size);
		set_cursor(2, 1);
		printk("[Z] - previous sector  [R] - exit        [N] - change current sector");
		set_cursor(2, 2);
		printk("[X] - next sector      [S] - save        [L] - load           [K] - clear");
		set_cursor(2, 3);
		printk("[O] - lower buffer     [P] - upper buffer");
		if (is_upper) {
			s_offset = 0x200;
		}
		else {
			s_offset = 0;
		}
		printk("\n        | 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
		for (int i = 0; i < 16; i++) {
			printk("\n    %04X|", s_offset + i * 16);
			for (int j = 0; j < 16; j++) {
				printk("%02X ", s_e_buffer[i*16+j]);
			}
		}
		set_cursor(c_offset%0x10*3+9,c_offset/0x10+5);
		out_char(s_e_buffer[c_offset + s_offset], 1);
		c = getch();
		switch (c) {
		case 'o':case 'O': {
			is_upper = 0;
			break;
		}
		case 'p':case 'P': {
			is_upper = 1;
			break;
		}
		case 'z':case 'Z': {
			if (s_edit_sector <= 0)s_edit_sector = 0; 
			else s_edit_sector--;
			break;
		}
		case 'x':case 'X': {
			s_edit_sector++;
			break;
		}
		case 'l':case 'L': {
			ata_read_sectors(0, 0, s_edit_sector, 1, s_e_buffer);
			break;
		}
		case 'y': {
			if (c_offset > 0) {
				c_offset--;
			}
			else {
				c_offset = 0;
			}
			break;
		}
		case 'Y': {
			if (c_offset > 16) {
				c_offset -= 16;
			}
			break;
		}
		case 'u': {
			if (c_offset < 255) {
				c_offset++;
			}
			else {
				c_offset = 255;
			}
			break;
		}
		case 'U': {
			if (c_offset < 255) {
				c_offset+=16;
			}
			else {
				c_offset = 255;
			}
			break;
		}
		case 'r':case 'R': {
			return;
			break;
		}
		}
	}
}
#endif
