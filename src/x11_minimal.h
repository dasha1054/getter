// x11_minimal.h - минимальные объявления X11 (собираем БЕЗ libX11-dev заголовков).
// Символы функций линкуются из системной libX11 (-lX11). Структуры объявлены как непрозрачные,
// доступ к полям событий идёт через byte-смещения (ABI Xlib, Linux x86-64).
#pragma once
#if !defined(_WIN32) && !defined(NO_X11)

#include <cstdint>
#include <cstddef>

typedef struct _XDisplay Display;
typedef unsigned long Atom;
typedef uint64_t Window;
typedef uint64_t Pixmap;
typedef uint64_t Cursor;
typedef int Screen;
typedef int Status;
typedef uint8_t Bool;
typedef unsigned long Time;
typedef uint64_t KeySym;
struct _XGC_impl; typedef struct _XGC_impl* GC;
struct Visual;
struct _XImage;

// ---- event masks ----
#define KeyPressMaskEv        (1L<<0)
#define KeyReleaseMaskEv      (1L<<1)
#define ButtonPressMaskEv     (1L<<2)
#define ButtonReleaseMaskEv   (1L<<3)
#define PointerMotionMaskEv   (1L<<6)
#define ExposureMaskEv        (1L<<15)
#define StructureNotifyMaskEv (1L<<17)

// ---- event codes ----
#define Ev_KeyPress        2
#define Ev_KeyRelease      3
#define Ev_ButtonPress     4
#define Ev_ButtonRelease   5
#define Ev_MotionNotify    6
#define Ev_ClientMessage   33

#define GrabModeAsync_     1L
#define CurrTime_          0L
#define Btn1_              1
#define Btn3_              3
#define ZPixmap_           0
#define NoneWin_           0L

// KeySyms (нужные)
#define XK_Space_       0x0020u
#define XK_Tab_         0xff09u
#define XK_Return_      0xff0du
#define XK_Escape_      0xff1bu
#define XK_Home_        0xff50u
#define XK_Left_        0xff51u
#define XK_Up_          0xff52u
#define XK_Right_       0xff53u
#define XK_Down_        0xff54u
#define XK_Prior_       0xff55u
#define XK_Next_        0xff56u
#define XK_End_         0xff57u
#define XK_Shift_L_     0xffe1u
#define XK_Shift_R_     0xffe2u
#define XK_Control_L_   0xffe3u
#define XK_Control_R_   0xffe4u

// ---- упрощённый XEvent: буфер + доступ по смещениям ----
union XEvent {
    char pad[192];
    int type;
};

// Смещения полей XKeyEvent/XButtonEvent/XMotionEvent (Linux x86-64):
//   int type @0 | unsigned long serial @8 | Bool send_event @16 | Display* @24
//   Window event @32 | child @40 | root @48 | subwindow @56 | Time @64
//   int x @72 | int y @76 | x_root @80 | y_root @84
//   unsigned state @88 | keycode|button @92 | same_screen @96
// XClientMessageEvent: window @32 | Atom message_type @40 | int format @48 | long data[5] @56
namespace xev {
inline int& type(XEvent& e) { return *(int*)&e.pad[0]; }
inline Window& window(XEvent& e) { return *(Window*)&e.pad[32]; }
inline int& x(XEvent& e) { return *(int*)&e.pad[72]; }
inline int& y(XEvent& e) { return *(int*)&e.pad[76]; }
inline unsigned& button(XEvent& e) { return *(unsigned*)&e.pad[92]; }
inline unsigned& keycode(XEvent& e) { return *(unsigned*)&e.pad[92]; }
inline Atom& messageType(XEvent& e) { return *(Atom*)&e.pad[40]; }
inline long& clientData0(XEvent& e) { return *(long*)&e.pad[56]; }
}

// ---- XImage: поля идут в порядке объявления (ABI Xlib). data - 12-е поле. ----
struct XImageFull {
    int width;
    int height;
    int xoffset;
    int format;
    char* data;
};

// ---- функции Xlib (линкуемся с -lX11) ----
extern "C" {
Display* XOpenDisplay(const char*);
Status XCloseDisplay(Display*);
int DefaultScreen(Display*);
Window RootWindow(Display*, int);
unsigned long BlackPixel(Display*, int);
Visual* DefaultVisual(Display*, int);
int XDefaultDepth(Display*, int);
Window XCreateSimpleWindow(Display*, Window, int, int, unsigned int, unsigned int,
                           unsigned int, unsigned long, unsigned long);
Status XStoreName(Display*, Window, const char*);
Status XSelectInput(Display*, Window, long);
Atom XInternAtom(Display*, const char*, Bool);
Status XSetWMProtocols(Display*, Window, Atom*, int);
Status XMapRaised(Display*, Window);
Status XSync(Display*, Bool);
GC XCreateGC(Display*, Window, unsigned long, void*);
int XPending(Display*);
int XNextEvent(Display*, XEvent*);
KeySym XKeycodeToKeysym(Display*, unsigned, int);
Status XGrabPointer(Display*, Window, Bool, unsigned, int, int, Window, Cursor, Time);
Status XUngrabPointer(Display*, Time);
Status XWarpPointer(Display*, Window, Window, int, int, unsigned, unsigned, int, int);
struct _XImage* XCreateImage(Display*, Visual*, unsigned, unsigned, int, char*,
                             unsigned, unsigned, int, int);
int XDestroyImage(struct _XImage*);
Status XPutImage(Display*, Window, GC, struct _XImage*, int, int, int, int, unsigned, unsigned);
Status XFlush(Display*);
Cursor XCreateFontCursor(Display*, unsigned);
Status XDefineCursor(Display*, Window, Cursor);
}

#endif // !WIN32 && !NO_X11
