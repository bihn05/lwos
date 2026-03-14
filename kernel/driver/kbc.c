#include <driver/kbc.h>
#include <printk.h>
#include <pic.h>

keyboard_buffer_t kbd_buffer;

static inline uint8_t kbc_status(void) {
    return inb(KEYBOARD_CMD);   // 0x64
}
void kbc_init() {
    kb_buffer_init();
    IRQ_clear_mask(1);
    while (kbc_status() & 0x01) {
        (void)inb(KEYBOARD_DAT);   // 0x60
    }
    printk("Keyboard controller initialized.\n");
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
    kbd_buffer.left_shift = false;
    kbd_buffer.right_shift = false;
    kbd_buffer.left_ctrl = false;
    kbd_buffer.right_ctrl = false;
    kbd_buffer.left_alt = false;
    kbd_buffer.right_alt = false;

    kbd_buffer.extended_key_pending = false;
    kbd_buffer.extended_code = 0;

    printk("Keyboard buffer initialized.\n");
}

void kbc_interrupt_handler() {
	uint8_t sc = inb(KEYBOARD_DAT);
    uint8_t ascii = 0;

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
                wake_up_one(&kbd_wait_queue);
            }
            break;
        }
    }
}

uint8_t sc_to_ascii(uint8_t sc, bool shift, bool caps_lock) {
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
    return 0;
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

char getch(void) {
    for (;;) {
        __asm__ volatile("cli");

        if (!kb_buffer_is_emtpy(&kbd_buffer)) {
            char c = buffer_get(&kbd_buffer);
            __asm__ volatile("sti");
            return c;
        }

        thread_block_on(&kbd_wait_queue);
        /* 被唤醒后会回到这里，然后重新检查缓冲区 */
    }
}
int try_getch(void) {
	if (kb_buffer_is_emtpy(&kbd_buffer)) {
		return -1;
	}
	return (int)buffer_get(&kbd_buffer);
}
bool kbhit(void) {
	return !kb_buffer_is_emtpy(&kbd_buffer);
}