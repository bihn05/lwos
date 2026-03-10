#include <driver/tty.h>
#include <driver/kbc.h>
#include <graphics.h>

static void tty_draw_cursor(tty_t *tty) {
    uint32_t px = tty->cursor_x * tty->char_w;
    uint32_t py = tty->cursor_y * tty->char_h;

    tty->surface->ops->draw_rect(tty->surface,
                  1,
                  px,
                  py + tty->char_h - 16,
                  tty->char_w,
                  16,
                  tty->fg);
}
static void tty_erase_cursor(tty_t *tty) {
    uint32_t px = tty->cursor_x * tty->char_w;
    uint32_t py = tty->cursor_y * tty->char_h;


    tty->surface->ops->draw_rect(tty->surface,
                  1,
                  px,
                  py + tty->char_h - 16,
                  tty->char_w,
                  16,
                  tty->bg);
}
void tty_init(tty_t *tty, gfx_surface_t *surface,
              uint32_t fg, uint32_t bg,
              uint32_t char_w, uint32_t char_h) {
    if (!tty || !surface || char_w == 0 || char_h == 0) {
        return;
    }

    memset(tty, 0, sizeof(*tty));

    tty->surface = surface;
    tty->fg = fg;
    tty->bg = bg;
    tty->char_w = char_w;
    tty->char_h = char_h;

    tty->cols = surface->width / char_w;
    tty->rows = surface->height / char_h;

    tty->cursor_x = 0;
    tty->cursor_y = 0;
    tty->line_len = 0;
    tty->line_ready = false;

    tty_clear(tty);
}

void tty_clear(tty_t *tty) {
    if (!tty || !tty->surface) return;

    tty->surface->ops->fill(tty->surface, tty->bg);
    tty->cursor_x = 0;
    tty->cursor_y = 0;
    tty_draw_cursor(tty);
}

void tty_newline(tty_t *tty) {
    if (!tty) return;

    tty_erase_cursor(tty);

    tty->cursor_x = 0;
    tty->cursor_y++;

    // 先做最简单策略：到底了就清屏
    if (tty->cursor_y >= tty->rows) {
        tty_clear(tty);
        return;
    }

    tty_draw_cursor(tty);
}

void tty_backspace(tty_t *tty) {
    if (!tty) return;

    if (tty->cursor_x == 0) {
        return;
    }

    tty_erase_cursor(tty);

    tty->cursor_x--;

    uint32_t px = tty->cursor_x * tty->char_w;
    uint32_t py = tty->cursor_y * tty->char_h;

    // 把当前位置字符擦掉
    tty->surface->ops->draw_rect(tty->surface, 1, px, py, tty->char_w, tty->char_h, tty->bg);

    tty_draw_cursor(tty);
}

void tty_putc(tty_t *tty, char c) {
    if (!tty || !tty->surface) return;

    if (c == '\n') {
        tty_newline(tty);
        return;
    }

    if (c == '\r') {
        tty_erase_cursor(tty);
        tty->cursor_x = 0;
        tty_draw_cursor(tty);
        return;
    }

    if (c == '\b') {
        tty_backspace(tty);
        return;
    }

    tty_erase_cursor(tty);

    uint32_t px = tty->cursor_x * tty->char_w;
    uint32_t py = tty->cursor_y * tty->char_h;

    tty->surface->ops->draw_char(tty->surface, px, py, c, tty->fg, tty->bg);

    tty->cursor_x++;

    if (tty->cursor_x >= tty->cols) {
        tty->cursor_x = 0;
        tty->cursor_y++;
        if (tty->cursor_y >= tty->rows) {
            tty_clear(tty);
            return;
        }
    }

    tty_draw_cursor(tty);
}

void tty_write(tty_t *tty, const char *s) {
    if (!tty || !s) return;

    while (*s) {
        tty_putc(tty, *s++);
    }
}

void tty_handle_input(tty_t *tty, char c) {
    if (!tty) return;

    if (c == '\r') {
        c = '\n';
    }

    if (c == '\b') {
        if (tty->line_len > 0) {
            tty->line_len--;
            tty->linebuf[tty->line_len] = '\0';
            tty_backspace(tty);
        }
        return;
    }

    if (c == '\n') {
        if (tty->line_len < TTY_LINEBUF_SIZE) {
            tty->linebuf[tty->line_len] = '\0';
        } else {
            tty->linebuf[TTY_LINEBUF_SIZE - 1] = '\0';
        }

        tty_putc(tty, '\n');
        tty->line_ready = true;
        return;
    }

    if (tty->line_len + 1 < TTY_LINEBUF_SIZE) {
        tty->linebuf[tty->line_len++] = c;
        tty->linebuf[tty->line_len] = '\0';
        tty_putc(tty, c);
    }
}

int tty_readline(tty_t *tty, char *out, uint32_t maxlen) {
    if (!tty || !out || maxlen == 0) {
        return -1;
    }

    tty->line_len = 0;
    tty->line_ready = false;
    tty->linebuf[0] = '\0';

    while (!tty->line_ready) {
        char c = getch();
        tty_handle_input(tty, c);
    }

    uint32_t n = tty->line_len;
    if (n >= maxlen) {
        n = maxlen - 1;
    }

    memcpy(out, tty->linebuf, n);
    out[n] = '\0';

    tty->line_len = 0;
    tty->line_ready = false;
    tty->linebuf[0] = '\0';

    return (int)n;
}