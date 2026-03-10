// windows manager

#ifndef _WINDOWS_H
#define _WINDOWS_H

#include <graphics.h>

typedef struct window;

typedef struct {
    void (*draw)(window_t *win);
    void (*handle_event)(window_t *win, void *event);
} window_ops_t;

typedef struct window {
    int x, y, width, height;
    const char *title;
    void *content; // 可以是任何类型，取决于窗口的用途
    const window_ops_t *ops;
} window_t;

#endif