#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "xwm.h"

static bool running = true;

void frame(Window wnd) {
    XWindowAttributes attribs;
    if (!XGetWindowAttributes(_display, wnd, &attribs))
        printf("xwm: warning: failed to get window attributes\n");

    const Window frame = XCreateSimpleWindow(
        _display,
        attribs.root,
        attribs.x + 10,
        attribs.y + 10,
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
        -1, -1
    );

    XMapWindow(_display, frame);

    XGrabKey(_display, XKeysymToKeycode(_display, XK_F4), Mod1Mask, wnd, True, GrabModeAsync, GrabModeAsync);
    XGrabButton(_display, Button1, AnyModifier, wnd, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
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
    XMapWindow(_display, event.window);
    frame(event.window);
}

void keypress(XKeyEvent event) {
    if ((event.state & Mod1Mask) &&
        (event.keycode == XKeysymToKeycode(_display, XK_F4)))
    {
        printf("xwm: debug: alt+f4 pressed\n");
    }
}

void buttonpress(XButtonEvent event) {
    printf("xwm: debug: button pressed\n");

    if (event.window != None && event.window != PointerRoot) {
        XSetInputFocus(_display, event.window, RevertToPointerRoot, CurrentTime);
        XRaiseWindow(_display, event.window);
        XFlush(_display);
    }
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
            case KeyPress:
                keypress(event.xkey);
                break;
            case ButtonPress:
                buttonpress(event.xbutton);
                break;
            case CreateNotify:
                break;
            default:
                printf("xwm: warning: ignored event type %d\n", event.type);
                break;
        }
    }
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