#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <stdint.h>
#include <driver/io.h>
#include <interrupt.h>

#define KEYBOARD_DAT 0x60
#define KEYBOARD_CMD 0x64

#define KB_BUFFER_SIZE 256
typedef enum {
    KBD_STATE_NORMAL,
    KBD_STATE_EXTENDED,
    KBD_STATE_EXTENDED_2
} kbd_state_t;
typedef struct {
    uint8_t make_code;
    uint8_t break_code;
    const char* name;
    uint8_t normal_ascii;
    uint8_t shift_ascii;
} extended_key_t;
typedef struct {
	uint8_t sc;
    char normal;
    char shift;
    char ctrl;
    char alt;
} keymap_entry_t ;
static const keymap_entry_t keymap[] = {
    {0x1E, 'a', 'A', 0x01, 0x1E},  // A
    {0x30, 'b', 'B', 0x02, 0x30},  // B
    {0x2E, 'c', 'C', 0x03, 0x2E},  // C
    {0x20, 'd', 'D', 0x04, 0x20},  // D
    {0x12, 'e', 'E', 0x05, 0x12},  // E
    {0x21, 'f', 'F', 0x06, 0x21},  // F
    {0x22, 'g', 'G', 0x07, 0x22},  // G
    {0x23, 'h', 'H', 0x08, 0x23},  // H
    {0x17, 'i', 'I', 0x09, 0x17},  // I
    {0x24, 'j', 'J', 0x0A, 0x24},  // J
    {0x25, 'k', 'K', 0x0B, 0x25},  // K
    {0x26, 'l', 'L', 0x0C, 0x26},  // L
    {0x32, 'm', 'M', 0x0D, 0x32},  // M
    {0x31, 'n', 'N', 0x0E, 0x31},  // N
    {0x18, 'o', 'O', 0x0F, 0x18},  // O
    {0x19, 'p', 'P', 0x10, 0x19},  // P
    {0x10, 'q', 'Q', 0x11, 0x10},  // Q
    {0x13, 'r', 'R', 0x12, 0x13},  // R
    {0x1F, 's', 'S', 0x13, 0x1F},  // S
    {0x14, 't', 'T', 0x14, 0x14},  // T
    {0x16, 'u', 'U', 0x15, 0x16},  // U
    {0x2F, 'v', 'V', 0x16, 0x2F},  // V
    {0x11, 'w', 'W', 0x17, 0x11},  // W
    {0x2D, 'x', 'X', 0x18, 0x2D},  // X
    {0x15, 'y', 'Y', 0x19, 0x15},  // Y
    {0x2C, 'z', 'Z', 0x1A, 0x2C},  // Z

    {0x02, '1', '!', 0x00, 0x78},  // 1
    {0x03, '2', '@', 0x00, 0x79},  // 2
    {0x04, '3', '#', 0x00, 0x7A},  // 3
    {0x05, '4', '$', 0x00, 0x7B},  // 4
    {0x06, '5', '%', 0x00, 0x7C},  // 5
    {0x07, '6', '^', 0x00, 0x7D},  // 6
    {0x08, '7', '&', 0x00, 0x7E},  // 7
    {0x09, '8', '*', 0x00, 0x7F},  // 8
    {0x0A, '9', '(', 0x00, 0x80},  // 9
    {0x0B, '0', ')', 0x00, 0x81},  // 0

    {0x1C, '\n', '\n', 0x0A, 0x00}, // Enter
    {0x0E, '\b', '\b', 0x7F, 0x00}, // Backspace
    {0x39, ' ', ' ', 0x00, 0x00},   // Space
    {0x0F, '\t', '\t', 0x00, 0x00}, // Tab

    {0x00, 0, 0, 0, 0}
};
static const extended_key_t extended_keymap[] = {
    {0x47, 0xC7, "Home",       0, 0},
    {0x48, 0xC8, "Up",         0, 0},
    {0x49, 0xC9, "PageUp",     0, 0},
    {0x4B, 0xCB, "Left",       0, 0},
    {0x4D, 0xCD, "Right",      0, 0},
    {0x4F, 0xCF, "End",        0, 0},
    {0x50, 0xD0, "Down",       0, 0},
    {0x51, 0xD1, "PageDown",   0, 0},
    {0x52, 0xD2, "Insert",     0, 0},
    {0x53, 0xD3, "Delete",     0, 0},

    {0x1C, 0x9C, "KP_Enter",   '\n', '\n'}, 
    {0x35, 0xB5, "KP_Slash",   '/', '/'},

    {0x5B, 0xDB, "Left_Win",   0, 0},
    {0x5C, 0xDC, "Right_Win",  0, 0},
    {0x5D, 0xDD, "Menu",       0, 0},

    {0x00, 0x00, NULL, 0, 0}
};
#define KEY_EXTENDED_PREFIX    0xE0
#define KEY_EXTENDED_PREFIX2   0xE1 
#define KEY_RELEASE_PREFIX     0xF0

#define ASCII_EXT_HOME         0x97
#define ASCII_EXT_UP           0x98
#define ASCII_EXT_DOWN         0x99
#define ASCII_EXT_LEFT         0x9A
#define ASCII_EXT_RIGHT        0x9B
#define ASCII_EXT_INSERT       0x9C
#define ASCII_EXT_DELETE       0x9D
#define ASCII_EXT_PAGEUP       0x9E
#define ASCII_EXT_PAGEDOWN     0x9F

typedef struct {
    char buffer[KB_BUFFER_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
    kbd_state_t state;

    bool caps_lock;
    bool num_lock;
    bool scroll_lock;
    bool shift_pressed;
    bool ctrl_pressed;
    bool alt_pressed;
    bool left_shift;
    bool right_shift;
    bool left_ctrl;
    bool right_ctrl;
    bool left_alt;
    bool right_alt;

    bool extended_key_pending;
    uint8_t extended_code;
} keyboard_buffer_t;
static keyboard_buffer_t kbd_buffer;

char ascii;
extern void set_interrupt_mask(uint8_t vector, uint8_t status);
extern void test(int a, int b);
extern void keyboard_hd(void);
char sc_to_ascii(uint8_t sc, bool shift, bool caps_lock);
bool buffer_put(keyboard_buffer_t* buf, char c);
char buffer_get(keyboard_buffer_t* buf);
bool kb_buffer_is_emtpy(keyboard_buffer_t* buf);
void keyboard_handler(void) {
	send_eoi(1);
	uint16_t sc = inb(KEYBOARD_DAT);
//	printk("Keyboard Input 0x%02x\n", sc);

    bool key_released = (sc & 0x80);
    uint8_t key_code = sc & 0x7f;
    if (key_released) {
        switch (key_code) {
        case 0x2a:
        case 0x36:
            kbd_buffer.shift_pressed = false;
            break;
        case 0x1d:
            kbd_buffer.ctrl_pressed = false;
            break;
        case 0x38:
            kbd_buffer.alt_pressed = false;
            break;
        }
    }
    else {
        switch (key_code) {
        case 0x2a:
        case 0x36:
            kbd_buffer.shift_pressed = true;
            break;
        case 0x1d:
            kbd_buffer.ctrl_pressed = true;
            break;
        case 0x38:
            kbd_buffer.alt_pressed = true;
            break;
        case 0x3a:
            kbd_buffer.caps_lock = !kbd_buffer.caps_lock;
            break;
        default:
            ascii = sc_to_ascii(
                key_code,
                kbd_buffer.shift_pressed,
                kbd_buffer.caps_lock
            );
            if (ascii != 0) {
                buffer_put(&kbd_buffer, ascii);
            }
            break;
        }
    }
}
void kb_buffer_init() {
    kbd_buffer.head = 0;
    kbd_buffer.tail = 0;
    kbd_buffer.count = 0;
    kbd_buffer.state = KBD_STATE_NORMAL;
    kbd_buffer.caps_lock = false;
    kbd_buffer.num_lock = true;
    kbd_buffer.scroll_lock = false;
    kbd_buffer.shift_pressed = false;
    kbd_buffer.ctrl_pressed = false;
    kbd_buffer.alt_pressed = false;
    kbd_buffer.extended_key_pending = false;
    kbd_buffer.extended_code = 0;
}
bool buffer_put(keyboard_buffer_t* buf, char c) {
	if (buf->count >= KB_BUFFER_SIZE) {
		return false;
	}

	buf->buffer[buf->head] = c;
	buf->head = (buf->head + 1) % KB_BUFFER_SIZE;
	buf->count++;
	return true;
}
char buffer_get(keyboard_buffer_t* buf) {
	if (buf->count == 0) {
		return 0;
	}
	
	char c = buf->buffer[buf->tail];
	buf->tail = (buf->tail + 1) % KB_BUFFER_SIZE;
	buf->count--;
	return c;
}
bool kb_buffer_is_emtpy(keyboard_buffer_t* buf) {
	return buf->count == 0;
}
char sc_to_ascii(uint8_t sc, bool shift, bool caps_lock) {
    for (int i = 0; keymap[i].sc != 0; i++) {
        if (keymap[i].sc == sc) {
            if (shift || caps_lock) {
                return keymap[i].shift;
            }
            else {
                return keymap[i].normal;
            }
        }
    }
}
void keyboard_init() {
	kb_buffer_init(&kbd_buffer);
	idt_set_gate(0x21, (uint32_t)keyboard_hd, 0x08, 0x8e);
	idt_flush();
	IRQ_clear_mask(1);
}


#endif
