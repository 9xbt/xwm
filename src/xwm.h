#include <X11/Xlib.h>

typedef struct {
    Window window;
    Window frame;
} frame_t;

void kill(Window wnd);
void die(const char *fmt, ...);
int wm_running(Display *display, XErrorEvent *err);
int xerror(Display *_display, XErrorEvent *err);
void run(void);