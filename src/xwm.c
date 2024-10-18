#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include "xwm.h"

Display *_display;

static bool running = true;
static frame_t *frames = NULL;
static size_t frame_no = 0;

void frame(Window wnd) {
    XWindowAttributes attribs;
    if (!XGetWindowAttributes(_display, wnd, &attribs))
        printf("xwm: warning: failed to get window attributes\n");

    Window frame = XCreateSimpleWindow(
        _display,
        attribs.root,
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
        SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask | ExposureMask
    );

    XAddToSaveSet(_display, wnd);

    XReparentWindow(
        _display,
        wnd,
        frame,
        -1, -1
    );

    XMapWindow(_display, frame);

    frames = realloc(frames, sizeof(frame_t) * (frame_no + 1));
    frames[frame_no].window = wnd;
    frames[frame_no].frame = frame;
    frame_no++;

    XGrabKey(_display, XKeysymToKeycode(_display, XK_F4), Mod1Mask, wnd, True, GrabModeAsync, GrabModeAsync);
    XGrabButton(_display, Button1, AnyModifier, frame, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
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

        Atom wm_delete = XInternAtom(_display, "WM_DELETE_WINDOW", False);
        
        XEvent e;
        e.xclient.type = ClientMessage;
        e.xclient.serial = 0;
        e.xclient.send_event = True;
        e.xclient.window = event.window;
        e.xclient.message_type = XInternAtom(_display, "WM_PROTOCOLS", True);
        e.xclient.format = 32;
        e.xclient.data.l[0] = wm_delete;
        e.xclient.data.l[1] = CurrentTime;

        XSendEvent(_display, event.window, False, NoEventMask, &e);
        XFlush(_display);
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
            case DestroyNotify:
                for (size_t i = 0; i < frame_no; i++) {
                    if (event.xdestroywindow.window == frames[i].window) {
                        XDestroyWindow(_display, frames[i].frame);
                        break;
                    }
                }
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