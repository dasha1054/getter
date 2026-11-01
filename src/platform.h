// platform.h - кроссплатформенный слой: окно, ввод, вывод кадра. Без внешних библиотек.
// Windows: GDI + raw-ish mouse (SetWindowsPos для relative). Linux: X11 без заголовков (собственные объявления).
#pragma once
#include <cstdint>

#ifndef _WIN32
// Минимальные объявления X11 (см. x11_minimal.h) — собираем без libX11-dev заголовков,
// линкуемся с системной libX11.
#include "x11_minimal.h"
#endif

struct WindowHandle {
    void* native = nullptr; // HWND или Display*
    int width = 0, height = 0;
    const char* title = "QUAKELIKE";
};

class Platform {
public:
    bool initWindow(int w, int h, const char* title);
    void shutdown();
    bool pumpEvents();          // false если окно закрыто
    void present(const uint32_t* framebuffer, int w, int h);
    bool mouseLocked = true;
    void setMouseGrab(bool grab);
    // опрашивается game loop-ом
    struct Events {
        float dx = 0, dy = 0;
        bool lmb = false, rmb = false;
        bool lmbPressed = false, rmbPressed = false;
        int wheel = 0;
        bool keyDown[512] = {false};
        bool quit = false;
    };
    Events events;

    bool key(int kc) const { return events.keyDown[kc & 511]; }

    // виртуальные коды (унифицированные)
    enum Key : int {
        K_W = 1, K_A, K_S, K_D, K_SPACE, K_LMB, K_RMB, K_ESCAPE, K_ENTER,
        K_1, K_2, K_3, K_4, K_5, K_6, K_7, K_8, K_9, K_R, K_TAB, K_M, K_P,
        K_LEFT, K_RIGHT, K_UP, K_DOWN, K_Q, K_E, K_SHIFT, K_CTRL,
        K_BRACKET_L, K_BRACKET_R, // [ и ] — смена уровня как changelevel
        K_COUNT
    };

private:
#ifdef _WIN32
    void* hdcData = nullptr;
    void* bitmapInfo = nullptr;
#else
    void* xdisplay = nullptr;
    void* xwindow = nullptr;
    void* gc = nullptr;
    void* image = nullptr;
#endif
};

double getTimeSeconds();
void sleepMs(int ms);
