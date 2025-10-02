#ifndef _KERYBOARD_CONTROLLER_H
#define _KERYBOARD_CONTROLLER_H

#include <stdint.h>
#include <driver/io.h>
#include <interrupt.h>
#include <kernel.h>

#define KB_BUFFER_SIZE 256
#define SCANCODE_BUFFER_SIZE 32

typedef struct {
	bool shift : 1;
	bool ctrl : 1;
	bool alt : 1;
	bool caps_lock : 1;
	bool num_lock : 1;
	bool scroll_lock : 1;
	bool left_shift : 1;
	bool right_shift : 1;
	bool left_ctrl : 1;
	bool right_ctrl : 1;
	bool left_alt : 1;
	bool right_alt : 1;
} kb_flags_t;
typedef struct {
	uint8_t buffer[KB_BUFFER_SIZE];
	uint16_t head;
	uint16_t tail;
	uint16_t count;

	uint8_t scancode_buffer[SCANCODE_BUFFER_SIZE];
	uint16_t sc_head;
	uint16_t sc_tail;
	uint16_t sc_count;

	kb_flags_t flags;
	uint8_t last_scancode;
	bool extended; // Indicates if the next scancode is extended (0xE0 prefix)
	bool break_code; // Indicates if the next scancode is a break code (key release)
} kb_driver_t;
kb_driver_t kbd;
const char keymap_normal[128] = {
	0,   27,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
	'\t','q',  'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
	0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
	0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
	'*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	'7', '8',  '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.'
};
const char keymap_shift[128] = {
	0,   27,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
	'\t','Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
	0,   'A',  'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
	0,   '|',  'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
	'*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	'7', '8',  '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.'
};

#define KEY_ESC        0x01
#define KEY_F1         0x3B
#define KEY_F2         0x3C
#define KEY_F3         0x3D
#define KEY_F4         0x3E
#define KEY_F5         0x3F
#define KEY_F6         0x40
#define KEY_F7         0x41
#define KEY_F8         0x42
#define KEY_F9         0x43
#define KEY_F10        0x44
#define KEY_F11        0x57
#define KEY_F12        0x58

#define KEY_UP         0x48
#define KEY_DOWN       0x50
#define KEY_LEFT       0x4B
#define KEY_RIGHT      0x4D
#define KEY_HOME       0x47
#define KEY_END        0x4F
#define KEY_PGUP       0x49
#define KEY_PGDN       0x51
#define KEY_INSERT     0x52
#define KEY_DELETE     0x53

void handle_function_keys(uint8_t scancode);
char handle_ctrl_combinations(char c);
void init_kb();
void put_char_buffer(char c);
void put_scancode_buffer(uint8_t scancode);
char get_char_buffer();
uint8_t get_scancode_buffer();
void keyboard_disable();
void keyboard_enable();
void keyboard_handler(void); // interrupt service

void handle_special_keys(uint8_t scancode) {
	if (kbd.break_code) {
		switch (scancode) {
		case 0x2A: kbd.flags.left_shift = false; break;
		case 0x36: kbd.flags.right_shift = false; break;
		case 0x1D: kbd.flags.left_ctrl = false; break;
		case 0xE01D: kbd.flags.right_ctrl = false; break;
		case 0x38: kbd.flags.left_alt = false; break;
		case 0xE038: kbd.flags.right_alt = false; break;
		}
	}
	else {
		switch (scancode) {
		case 0x2A: kbd.flags.left_shift = true; break;
		case 0x36: kbd.flags.right_shift = true; break;
		case 0x1D: kbd.flags.left_ctrl = true; break;
		case 0xE01D: kbd.flags.right_ctrl = true; break;
		case 0x38: kbd.flags.left_alt = true; break;
		case 0xE038: kbd.flags.right_alt = true; break;
		case 0x3A: kbd.flags.caps_lock ^= true; break;
		case 0x45: kbd.flags.num_lock ^= true; break;
		case 0x46: kbd.flags.scroll_lock ^= true; break;
		}
	}

	kbd.flags.shift = kbd.flags.left_shift || kbd.flags.right_shift;
	kbd.flags.ctrl = kbd.flags.left_ctrl || kbd.flags.right_ctrl;
	kbd.flags.alt = kbd.flags.left_alt || kbd.flags.right_alt;
}
void process_scancode(uint8_t scancode) {
	char c = 0;

	const char* current_keymap = (kbd.flags.shift ^ kbd.flags.caps_lock) ?
		keymap_shift : keymap_normal;

	if (scancode < 128) {
		c = current_keymap[scancode];
	}

	if (c == 0) {
		handle_function_keys(scancode);
	} else {
		if (kbd.flags.ctrl) {
			c = handle_ctrl_combinations(c);
		}
		if (c != 0) {
			put_char_buffer(c);
		}

		put_scancode_buffer(scancode);
	}
}
void handle_function_keys(uint8_t scancode) {
	switch (scancode) {
	case KEY_UP:
	case KEY_DOWN:
	case KEY_LEFT:
	case KEY_RIGHT:
		put_char_buffer(0x18);
		put_char_buffer('[');
		switch (scancode) {
		case KEY_UP: put_char_buffer('A'); break;
		case KEY_DOWN: put_char_buffer('B'); break;
		case KEY_LEFT: put_char_buffer('C'); break;
		case KEY_RIGHT: put_char_buffer('D'); break;
		}
		break;
	}
}
char handle_ctrl_combinations(char c) {
	if (c >= 'a' && c <= 'z') {
		return c - 'a' + 1;
	}
	if (c >= 'A' && c <= 'Z') {
		return c - 'A' + 1;;
	}
	return c;
}
void init_kb() {
	memset(&kbd, 0, sizeof(kbd));

	init_keyboard_hardware();
}
void put_char_buffer(char c) {
	if (kbd.count < KB_BUFFER_SIZE) {
		kbd.buffer[kbd.head] = c;
		kbd.head = (kbd.head + 1) % KB_BUFFER_SIZE;
		kbd.count++;
	}
}
void put_scancode_buffer(uint8_t scancode) {
	if (kbd.sc_count < KB_BUFFER_SIZE) {
		kbd.scancode_buffer[kbd.sc_head] = scancode;
		kbd.sc_head = (kbd.sc_head + 1) % SCANCODE_BUFFER_SIZE;
		kbd.sc_count++;
	}
}
char get_char_buffer() {
	if (kbd.count == 0) {
		return 0;
	}

	char c = kbd.buffer[kbd.tail];
	kbd.tail = (kbd.tail + 1) & KB_BUFFER_SIZE;
	kbd.count--;

	return c;
}
uint8_t get_scancode_buffer() {
	if (kbd.sc_count == 0) {
		return 0;
	}

	uint8_t scancode = kbd.scancode_buffer[kbd.sc_tail];
	kbd.sc_tail = (kbd.sc_tail + 1) % SCANCODE_BUFFER_SIZE;
	kbd.sc_count--;

	return scancode;
}
void init_keyboard_hardware() {
	outb(inb(0x21 & 0xFD), 0x21);

	keyboard_disable();
	keyboard_enable();

	idt_set_gate(0x21, (uint32_t)keyboard_handler, 0x08, 0x8E);
	load_idt();
}
void keyboard_disable() {
	outb(0xAD, 0x64);
}
void keyboard_enable() {
	outb(0xAE, 0x64);
}
void keyboard_handler(void) {

	printk("hi");

	asm volatile("pusha");

	uint8_t scancode = inb(0x60);

	if (scancode == 0xE0) {
		kbd.extended = true;
		goto end;
	}
	if (scancode & 0x80) {
		kbd.break_code = true;
		scancode &= 0x7F;
	}
	else {
		kbd.break_code = false;
	}

	handle_special_keys(scancode);
	if (!kbd.break_code) {
		process_scancode(scancode);
	}
	kbd.extended = false;
end:
	outb(0x20, 0x20);

	asm volatile("popa");
	asm volatile("iret");
}
void kb_shutdown() {
	asm volatile("cli"); // Disable interrupts

	uint8_t status;

	do {
		status = inb(0x64); // Read status register
	} while (status & 0x02); // Wait until input buffer is empty

	outb(0xFE, 0x64); // Send command to write to mouse

	while (1) {
		asm volatile("hlt"); // Halt CPU until next interrupt
	}
}

#endif