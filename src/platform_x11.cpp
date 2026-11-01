// platform_x11.cpp - Linux-реализация окна/ввода на X11.
// Заголовки libX11 не нужны: объявления в x11_minimal.h, линкуемся с -lX11.
#if !defined(_WIN32) && !defined(NO_X11)
#include "platform.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

static Display* g_dpy = nullptr;
static Window g_win = 0;
static GC g_gc = nullptr;
static struct _XImage* g_img = nullptr;
static char* g_imgBuf = nullptr;
static int g_w = 0, g_h = 0;
static Atom wmDelete;
static bool g_grab = false;

static int mapKeysym(KeySym ks) {
    if (ks >= 'a' && ks <= 'z') ks -= 32; // регистронезависимо
    switch ((unsigned)ks) {
        case 'W': return Platform::K_W;
        case 'A': return Platform::K_A;
        case 'S': return Platform::K_S;
        case 'D': return Platform::K_D;
        case XK_Space_: return Platform::K_SPACE;
        case XK_Escape_: return Platform::K_ESCAPE;
        case XK_Return_: return Platform::K_ENTER;
        case '1': return Platform::K_1;
        case '2': return Platform::K_2;
        case '3': return Platform::K_3;
        case '4': return Platform::K_4;
        case '5': return Platform::K_5;
        case '6': return Platform::K_6;
        case '7': return Platform::K_7;
        case '8': return Platform::K_8;
        case '9': return Platform::K_9;
        case 'R': return Platform::K_R;
        case XK_Tab_: return Platform::K_TAB;
        case 'M': return Platform::K_M;
        case 'P': return Platform::K_P;
        case 'Q': return Platform::K_Q;
        case 'E': return Platform::K_E;
        case XK_Left_: return Platform::K_LEFT;
        case XK_Right_: return Platform::K_RIGHT;
        case XK_Up_: return Platform::K_UP;
        case XK_Down_: return Platform::K_DOWN;
        case XK_Shift_L_: case XK_Shift_R_: return Platform::K_SHIFT;
        case XK_Control_L_: case XK_Control_R_: return Platform::K_CTRL;
    }
    return -1;
}

bool Platform::initWindow(int w, int h, const char* title) {
    g_w = w; g_h = h;
    g_dpy = XOpenDisplay(nullptr);
    if (!g_dpy) { fprintf(stderr, "No X display. Run under X11/Wayland(XWayland).\n"); return false; }
    int scr = XDefaultScreen(g_dpy);
    Window root = XRootWindow(g_dpy, scr);
    g_win = XCreateSimpleWindow(g_dpy, root, 0, 0, (unsigned)w, (unsigned)h, 0,
                                XBlackPixel(g_dpy, scr), XBlackPixel(g_dpy, scr));
    XStoreName(g_dpy, g_win, title);
    XSelectInput(g_dpy, g_win,
                 ExposureMaskEv | KeyPressMaskEv | KeyReleaseMaskEv |
                 ButtonPressMaskEv | ButtonReleaseMaskEv | PointerMotionMaskEv |
                 StructureNotifyMaskEv);
    wmDelete = XInternAtom(g_dpy, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(g_dpy, g_win, &wmDelete, 1);
    g_gc = XCreateGC(g_dpy, g_win, 0, nullptr);
    XMapRaised(g_dpy, g_win);
    XSync(g_dpy, 0);
    setMouseGrab(true);
    return true;
}

void Platform::setMouseGrab(bool grab) {
    if (!g_dpy) return;
    g_grab = grab;
    unsigned mask = ButtonPressMaskEv | ButtonReleaseMaskEv | PointerMotionMaskEv;
    if (grab) {
        XGrabPointer(g_dpy, g_win, 0, mask, (int)GrabModeAsync_, (int)GrabModeAsync_,
                     g_win, 0, CurrTime_);
        XWarpPointer(g_dpy, NoneWin_, g_win, 0, 0, 0, 0, g_w / 2, g_h / 2);
    } else {
        XUngrabPointer(g_dpy, CurrTime_);
    }
    XSync(g_dpy, 0);
}

void Platform::shutdown() {
    if (g_imgBuf) free(g_imgBuf);
    g_imgBuf = nullptr;
    if (g_img) XDestroyImage(g_img);
    g_img = nullptr;
    if (g_dpy) XCloseDisplay(g_dpy);
    g_dpy = nullptr;
}

bool Platform::pumpEvents() {
    if (!g_dpy) { events.quit = true; return false; }
    while (XPending(g_dpy)) {
        XEvent e;
        XNextEvent(g_dpy, &e);
        auto& ev = events;
        switch (xev::type(e)) {
            case Ev_ClientMessage:
                if ((Atom)xev::clientData0(e) == wmDelete) ev.quit = true;
                break;
            case Ev_KeyPress:
            case Ev_KeyRelease: {
                KeySym ks = XKeycodeToKeysym(g_dpy, xev::keycode(e), 0);
                int k = mapKeysym(ks);
                if (k >= 0) ev.keyDown[k] = (xev::type(e) == Ev_KeyPress);
                break;
            }
            case Ev_ButtonPress: {
                unsigned b = xev::button(e);
                if (b == Btn1_) { ev.lmb = true; ev.lmbPressed = true; }
                if (b == Btn3_) { ev.rmb = true; ev.rmbPressed = true; }
                if (b == 4) ev.wheel += 1;
                if (b == 5) ev.wheel -= 1;
                break;
            }
            case Ev_ButtonRelease: {
                unsigned b = xev::button(e);
                if (b == Btn1_) ev.lmb = false;
                if (b == Btn3_) ev.rmb = false;
                break;
            }
            case Ev_MotionNotify: {
                int cx = g_w / 2, cy = g_h / 2;
                ev.dx += (float)(xev::x(e) - cx);
                ev.dy += (float)(xev::y(e) - cy);
                if (g_grab) XWarpPointer(g_dpy, NoneWin_, g_win, 0, 0, 0, 0, cx, cy);
                break;
            }
        }
    }
    if (events.quit) return false;
    return true;
}

void Platform::present(const uint32_t* fb, int w, int h) {
    if (!g_dpy) return;
    size_t bytes = (size_t)w * h * 4;
    if (!g_img || w != g_w || h != g_h) {
        if (g_img) XDestroyImage(g_img);
        Visual* vis = XDefaultVisual(g_dpy, XDefaultScreen(g_dpy));
        g_img = XCreateImage(g_dpy, vis, 24, ZPixmap_, 0, nullptr,
                             (unsigned)w, (unsigned)h, 32, 0);
        if (g_imgBuf) free(g_imgBuf);
        g_imgBuf = (char*)malloc(bytes);
        g_w = w; g_h = h;
    }
    if (!g_img || !g_imgBuf) return;
    memcpy(g_imgBuf, fb, bytes);
    // data - 5-е поле XImage (int w,int h,int xoff,int fmt,char* data) по ABI Xlib
    ((XImageFull*)g_img)->data = g_imgBuf;
    XPutImage(g_dpy, g_win, g_gc, g_img, 0, 0, 0, 0, (unsigned)w, (unsigned)h);
    XFlush(g_dpy);
}

double getTimeSeconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void sleepMs(int ms) { usleep(ms * 1000); }

#endif // !WIN32 && !NO_X11
