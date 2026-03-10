#ifndef _TTY_H
#define _TTY_H

#include <stdint.h>
#include <graphics.h>

#define TTY_LINEBUF_SIZE 256

typedef struct {
    gfx_surface_t *surface;

    uint32_t cols;
    uint32_t rows;

    uint32_t cursor_x;
    uint32_t cursor_y;

    uint32_t fg;
    uint32_t bg;

    uint32_t char_w;
    uint32_t char_h;

    char linebuf[TTY_LINEBUF_SIZE];
    uint32_t line_len;
    bool line_ready;
} tty_t;

void tty_init(tty_t *tty, gfx_surface_t *surface,
              uint32_t fg, uint32_t bg,
              uint32_t char_w, uint32_t char_h);

void tty_putc(tty_t *tty, char c);
void tty_write(tty_t *tty, const char *s);
void tty_backspace(tty_t *tty);
void tty_newline(tty_t *tty);
void tty_clear(tty_t *tty);

void tty_handle_input(tty_t *tty, char c);
int tty_readline(tty_t *tty, char *out, uint32_t maxlen);

#endif