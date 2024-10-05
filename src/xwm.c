#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include "xwm.h"

static bool running = true;

void die(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vfprintf(stderr, fmt, args);
    va_end(args);

    exit(1);
}

int wm_running(Display *display, XErrorEvent *err) {
    die("xwm: another window manager is running already");
    return -1;
}

int xerror(Display *_display, XErrorEvent *err) {
    return 0;
}

void run(void) {
    XEvent event;
    XSync(_display, False);
    while (running && !XNextEvent(_display, &event)) {
        switch (event.type) {
            default:
                printf("xwm: warning: unknown event type %d", event.type);
                break;
        }
    }
}

int main(int argc, char *argv[]) {
    /* try to open display */
    if (!(_display = XOpenDisplay(NULL)))
        die("xwm: cannot open display\n");

    /* check if another window manager is running already */
    XSetErrorHandler(wm_running);
    XSelectInput(_display, DefaultRootWindow(_display), SubstructureRedirectMask);
    XSync(_display, False);
    XSetErrorHandler(xerror);
    XSync(_display, False);

    /* main event loop */
    run();

    /* close display */
    XCloseDisplay(_display);

    return EXIT_SUCCESS;
}