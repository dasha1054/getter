// platform_x11.cpp - Linux X11 реализация (через dlopen libX11, чтобы не требовать заголовки при сборке)
#if !defined(_WIN32)
#include "platform.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <dlfcn.h>





static Platform* g_plat = nullptr;
static Display* g_dpy = nullptr;
static Window g_win = 0;
static GC g_gc = 0;
static XImage* g_img = nullptr;
static char* g_imgBuf = nullptr;
static int g_w = 0, g_h = 0;
static Atom wmDelete;
static bool g_grab = false;

static int mapKeysym(KeySym ks) {
    switch (ks) {
        case XK_w: case XK_W: return Platform::K_W;
        case XK_a: case XK_A: return Platform::K_A;
        case XK_s: case XK_S: return Platform::K_S;
        case XK_d: case XK_D: return Platform::K_D;
        case XK_space: return Platform::K_SPACE;
        case XK_Escape: return Platform::K_ESCAPE;
        case XK_Return: return Platform::K_ENTER;
        case XK_1: return Platform::K_1;
        case XK_2: return Platform::K_2;
        case XK_3: return Platform::K_3;
        case XK_4: return Platform::K_4;
        case XK_5: return Platform::K_5;
        case XK_6: return Platform::K_6;
        case XK_7: return Platform::K_7;
        case XK_8: return Platform::K_8;
        case XK_9: return Platform::K_9;
        case XK_r: case XK_R: return Platform::K_R;
        case XK_Tab: return Platform::K_TAB;
        case XK_m: case XK_M: return Platform::K_M;
        case XK_p: case XK_P: return Platform::K_P;
        case XK_Left: return Platform::K_LEFT;
        case XK_Right: return Platform::K_RIGHT;
        case XK_Up: return Platform::K_UP;
        case XK_Down: return Platform::K_DOWN;
        case XK_q: case XK_Q: return Platform::K_Q;
        case XK_e: case XK_E: return Platform::K_E;
        case XK_Shift_L: case XK_Shift_R: return Platform::K_SHIFT;
        case XK_Control_L: case XK_Control_R: return Platform::K_CTRL;
    }
    return -1;
}

bool Platform::initWindow(int w, int h, const char* title) {
    g_plat = this;
    g_w = w; g_h = h;
    g_dpy = XOpenDisplay(nullptr);
    if (!g_dpy) { fprintf(stderr, "No X display. Run under X11/Wayland(XWayland).\n"); return false; }
    int scr = DefaultScreen(g_dpy);
    g_win = XCreateSimpleWindow(g_dpy, RootWindow(g_dpy, scr), 0, 0, w, h, 0,
                                BlackPixel(g_dpy, scr), BlackPixel(g_dpy, scr));
    XStoreName(g_dpy, g_win, title);
    XSelectInput(g_dpy, g_win, ExposureMask | KeyPressMask | KeyReleaseMask |
                 ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);
    Atom typeHints = XInternAtom(g_dpy, "_MOTIF_WM_HINTS", False);
    g_gc = XCreateGC(g_dpy, g_win, 0, nullptr);
    wmDelete = XInternAtom(g_dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(g_dpy, g_win, &wmDelete, 1);
    XMapRaised(g_dpy, g_win);
    XSync(g_dpy, False);
    setMouseGrab(true);
    return true;
}

void Platform::setMouseGrab(bool grab) {
    if (!g_dpy) return;
    g_grab = grab;
    if (grab) {
        XGrabPointer(g_dpy, g_win, True, 0, GrabModeAsync, GrabModeAsync, g_win, None, CurrentTime);
        // warp в центр
        XWarpPointer(g_dpy, None, g_win, 0, 0, 0, 0, g_w / 2, g_h / 2);
    } else {
        XUngrabPointer(g_dpy, CurrentTime);
    }
    XSync(g_dpy, False);
}

void Platform::shutdown() {
    if (g_imgBuf) free(g_imgBuf);
    if (g_img) { XDestroyImage(g_img); }
    if (g_dpy) XCloseDisplay(g_dpy);
}

bool Platform::pumpEvents() {
    if (!g_dpy) { events.quit = true; return false; }
    while (XPending(g_dpy)) {
        XEvent e;
        XNextEvent(g_dpy, &e);
        auto& ev = events;
        switch (e.type) {
            case ClientMessage:
                if ((Atom)e.xclient.data.l[0] == wmDelete) ev.quit = true;
                break;
            case KeyPress:
            case KeyRelease: {
                KeySym ks = XLookupKeysym(&e.xkey, 0);
                int k = mapKeysym(ks);
                if (k >= 0) ev.keyDown[k] = (e.type == KeyPress);
                break;
            }
            case ButtonPress:
                if (e.xbutton.button == Button1) { ev.lmb = true; ev.lmbPressed = true; }
                if (e.xbutton.button == Button3) { ev.rmb = true; ev.rmbPressed = true; }
                if (e.xbutton.button == 4) ev.wheel += 1;
                if (e.xbutton.button == 5) ev.wheel -= 1;
                break;
            case ButtonRelease:
                if (e.xbutton.button == Button1) ev.lmb = false;
                if (e.xbutton.button == Button3) ev.rmb = false;
                break;
            case MotionNotify: {
                int cx = g_w / 2, cy = g_h / 2;
                ev.dx += (float)(e.xmotion.x - cx);
                ev.dy += (float)(e.xmotion.y - cy);
                if (g_grab) XWarpPointer(g_dpy, None, g_win, 0, 0, 0, 0, cx, cy);
                break;
            }
        }
    }
    if (events.quit) return false;
    return true;
}

void Platform::present(const uint32_t* fb, int w, int h) {
    if (!g_dpy) return;
    if (!g_img || w != g_w || h != g_h) {
        if (g_img) XDestroyImage(g_img);
        g_img = XCreateImage(g_dpy, DefaultVisual(g_dpy, DefaultScreen(g_dpy)), 24, ZPixmap,
                             0, nullptr, w, h, 32, 0);
        g_w = w; g_h = h;
    }
    size_t bytes = (size_t)w * h * 4;
    if (!g_imgBuf) g_imgBuf = (char*)malloc(bytes);
    memcpy(g_imgBuf, fb, bytes);
    g_img->data = g_imgBuf;
    XPutImage(g_dpy, g_win, g_gc, g_img, 0, 0, 0, 0, w, h);
    XFlush(g_dpy);
}

double getTimeSeconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void sleepMs(int ms) { usleep(ms * 1000); }

#endif
