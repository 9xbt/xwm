#include <X11/Xlib.h>

typedef struct {
    Window window;
    Window frame;
} frame_t;

int gettextprop(Window wnd, Atom atom, char *text, unsigned int size);
void run(void);
void die(const char *fmt, ...);
int wm_running(Display *display, XErrorEvent *err);
int xerror(Display *_display, XErrorEvent *err);