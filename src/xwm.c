#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include "xwm.h"

static bool running = true;

void frame(Window wnd) {
    XWindowAttributes attribs;
    if (!XGetWindowAttributes(_display, wnd, &attribs))
        printf("xwm: warning: failed to get window attributes\n");

    /* don't blame me for this spaghetti 1am code that isn't even finished */

    const Window frame = XCreateSimpleWindow(
        _display,
        frame,
        attribs.x,
        attribs.y,
        attribs.width,
        attribs.height,
        border_width,
        border_color,
        bg_color
    );

    XSelectInput(
        _display,
        frame,
        SubstructureRedirectMask | SubstructureNotifyMask
    );

    XAddToSaveSet(_display, wnd);

    XReparentWindow(
        _display,
        wnd,
        frame,
        0, 0
    );

    XMapWindow(_display, frame);
}

void configurerequest(XConfigureRequestEvent event) {
    XWindowChanges changes;
    changes.x = event.x;
    changes.y = event.y;
    changes.width = event.width;
    changes.height = event.height;
    changes.border_width = event.border_width;
    changes.sibling = event.above;
    changes.stack_mode = event.detail;
    
    XConfigureWindow(_display, event.window, event.value_mask, &changes);
}

void maprequest(XMapRequestEvent event) {
    frame(event.window);
    XMapWindow(_display, event.window);
}

void die(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vfprintf(stderr, fmt, args);
    va_end(args);

    exit(EXIT_FAILURE);
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
            case ConfigureRequest:
                configurerequest(event.xconfigurerequest);
                break;
            case MapRequest:
                maprequest(event.xmaprequest);
                break;
            case CreateNotify:
                break;
            default:
                printf("xwm: warning: ignored event type %d", event.type);
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