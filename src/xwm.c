#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include "xwm.h"

Display *_display;

static bool running = true;
static bool super_pressed = false, mouse_pressed = false, dragging = false;
static int drag_start_x, drag_start_y, drag_start_mouse_x, drag_start_mouse_y;
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
        attribs.width + 12,
        attribs.height + 32,
        0, 0,
        0xc0c0c0
    );

    XSelectInput(
        _display,
        frame,
        SubstructureRedirectMask | SubstructureNotifyMask | ExposureMask
    );

    XSelectInput(
        _display,
        wnd,
        SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask
    );

    XAddToSaveSet(_display, wnd);

    XReparentWindow(
        _display,
        wnd,
        frame,
        5, 25
    );

    XMapWindow(_display, frame);

    GC gc = XCreateGC(_display, frame, 0, NULL);
    XSetForeground(_display, gc, BlackPixel(_display, 0));
    XFreeGC(_display, gc);

    frames = realloc(frames, sizeof(frame_t) * (frame_no + 1));
    frames[frame_no].window = wnd;
    frames[frame_no].frame = frame;
    frame_no++;

    XGrabKey(_display, XKeysymToKeycode(_display, XK_F4), Mod1Mask, wnd, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(_display, XKeysymToKeycode(_display, XK_Alt_L), 0, wnd, True, GrabModeAsync, GrabModeAsync);
    XGrabButton(_display, Button1, AnyModifier, wnd, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
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
    if (event.keycode == XKeysymToKeycode(_display, XK_Alt_L))
        super_pressed = true;

    if ((event.state & Mod1Mask) &&
        (event.keycode == XKeysymToKeycode(_display, XK_F4)))
    {
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

void keyrelease(XKeyEvent event) {
    if (event.keycode == XKeysymToKeycode(_display, XK_Alt_L))
        super_pressed = false;
}

void buttonpress(XButtonEvent event) {
    mouse_pressed = true;

    int x, y, unused;
    unsigned int mask;
    Window root = DefaultRootWindow(_display), child;
    XQueryPointer(_display, root, &root, &child, &x, &y, &unused, &unused, &mask);

    if (event.window != None && event.window != PointerRoot) {
        XSetInputFocus(_display, getclientwindow(child), RevertToPointerRoot, CurrentTime);
        XRaiseWindow(_display, child);
        XFlush(_display);
    }
}

void buttonrelease(XButtonEvent event) {
    mouse_pressed = false;
}

void destroynotify(XDestroyWindowEvent event) {
    for (size_t i = 0; i < frame_no; i++) {
        if (event.window == frames[i].window) {
            XDestroyWindow(_display, frames[i].frame);
            break;
        }
    }
}

void motionnotify(XMotionEvent event) {
    if (super_pressed && mouse_pressed) {
        if (!dragging) {
            int x, y, unused;
            unsigned int mask;
            Window root = DefaultRootWindow(_display), child;
            XQueryPointer(_display, root, &root, &child, &x, &y, &unused, &unused, &mask);

            XWindowAttributes attribs;
            if (!XGetWindowAttributes(_display, child, &attribs))
                printf("xwm: warning: failed to get window attributes\n");

            drag_start_x = attribs.x;
            drag_start_y = attribs.y;
            drag_start_mouse_x = x;
            drag_start_mouse_y = y;
            dragging = true;
        }
    } else
        dragging = false;

    if (dragging) {
        int x, y, unused;
        unsigned int mask;
        Window root = DefaultRootWindow(_display), child;
        if (!XQueryPointer(_display, root, &root, &child, &x, &y, &unused, &unused, &mask))
            printf("xwm: warning: failed to get pointer position\n");

        XMoveWindow(_display, child, drag_start_x + (x - drag_start_mouse_x), drag_start_y + (y - drag_start_mouse_y));
    }
}

void expose(XExposeEvent event) {
    if (event.count == 0) {
        XWindowAttributes attribs;
        if (!XGetWindowAttributes(_display, event.window, &attribs))
            printf("xwm: warning: failed to get window attributes\n");

        GC gc = XCreateGC(_display, event.window, 0, NULL);

        XSetForeground(_display, gc, 0xdfdfdf);
        XDrawRectangle(_display, event.window, gc, 0, 0, attribs.width - 1, attribs.height - 1);
        XSetForeground(_display, gc, 0xffffff);
        XDrawRectangle(_display, event.window, gc, 1, 1, attribs.width - 3, attribs.height - 3);
        XSetForeground(_display, gc, 0x808080);
        XDrawRectangle(_display, event.window, gc, 4, 24, attribs.width - 9, attribs.height - 29);
        XSetForeground(_display, gc, 0x000080);
        XFillRectangle(_display, event.window, gc, 4, 4, attribs.width - 8, 18);
        XSetForeground(_display, gc, 0xffffff);
        XDrawString(_display, event.window, gc, 8, 17, "client", 6);

        XFreeGC(_display, gc);
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
            case KeyRelease:
                keyrelease(event.xkey);
                break;
            case ButtonPress:
                buttonpress(event.xbutton);
                break;
            case ButtonRelease:
                buttonrelease(event.xbutton);
                break;
            case DestroyNotify:
                destroynotify(event.xdestroywindow);
                break;
            case MotionNotify:
                motionnotify(event.xmotion);
                break;
            case Expose:
                expose(event.xexpose);
                break;
            default:
                printf("xwm: warning: ignored event type %d\n", event.type);
                break;
        }
    }
}

Window getclientwindow(Window frame) {
    Window root_return, parent_return;
    Window *children;
    unsigned int n;

    if (XQueryTree(_display, frame, &root_return, &parent_return, &children, &n)) {
        Window client_window = children[0];
        XFree(children);
        return client_window;
    }
    return None;
}

void die(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vfprintf(stderr, fmt, args);
    va_end(args);

    exit(EXIT_FAILURE);
}

int wm_running(Display *display, XErrorEvent *err) {
    die("xwm: another window manager is running already\n");
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

    /* receive mouse events */
    XGrabPointer(_display, DefaultRootWindow(_display), True, 
                 ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                 GrabModeAsync, GrabModeAsync, None, None, CurrentTime);

    /* set mouse cursor */
    XDefineCursor(_display, DefaultRootWindow(_display), XCreateFontCursor(_display, XC_arrow));

    /* set background color */
    Window root = DefaultRootWindow(_display);
    XSetWindowBackground(_display, root, 0x008080);
    XClearWindow(_display, root);

    /* main event loop */
    run();

    /* close display */
    XCloseDisplay(_display);

    return EXIT_SUCCESS;
}