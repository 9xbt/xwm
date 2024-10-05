#include <X11/Xlib.h>

Display *_display;

const unsigned int border_width = 1;
const unsigned long border_color = 0x005577;
const unsigned long bg_color = 0x222222;

void frame(Window wnd);
void configurerequest(XConfigureRequestEvent event);
void maprequest(XMapRequestEvent event);
void die(const char *fmt, ...);
int wm_running(Display *display, XErrorEvent *err);
int xerror(Display *_display, XErrorEvent *err);
void run(void);