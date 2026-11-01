// platform_win.cpp - Windows-реализация окна/ввода через GDI (без внешних библиотек)
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include "platform.h"

static Platform* g_plat = nullptr;
static int g_centerX = 0, g_centerY = 0;
static bool g_grab = false;
static POINT g_lastMouse;
static bool g_haveLast = false;

static int mapVK(int vk) {
    switch (vk) {
        case 'W': return Platform::K_W;
        case 'A': return Platform::K_A;
        case 'S': return Platform::K_S;
        case 'D': return Platform::K_D;
        case VK_SPACE: return Platform::K_SPACE;
        case VK_ESCAPE: return Platform::K_ESCAPE;
        case VK_RETURN: return Platform::K_ENTER;
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
        case VK_TAB: return Platform::K_TAB;
        case 'M': return Platform::K_M;
        case 'P': return Platform::K_P;
        case VK_LEFT: return Platform::K_LEFT;
        case VK_RIGHT: return Platform::K_RIGHT;
        case VK_UP: return Platform::K_UP;
        case VK_DOWN: return Platform::K_DOWN;
        case 'Q': return Platform::K_Q;
        case 'E': return Platform::K_E;
        case VK_SHIFT: return Platform::K_SHIFT;
        case VK_CONTROL: return Platform::K_CTRL;
    }
    return -1;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (!g_plat) return DefWindowProc(hwnd, msg, wp, lp);
    auto& ev = g_plat->events;
    switch (msg) {
        case WM_CLOSE:
        case WM_DESTROY:
            ev.quit = true;
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            int k = mapVK((int)wp);
            if (k >= 0) ev.keyDown[k] = true;
            return 0;
        }
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            int k = mapVK((int)wp);
            if (k >= 0) ev.keyDown[k] = false;
            return 0;
        }
        case WM_LBUTTONDOWN: ev.lmb = true; ev.lmbPressed = true; return 0;
        case WM_LBUTTONUP:   ev.lmb = false; return 0;
        case WM_RBUTTONDOWN: ev.rmb = true; ev.rmbPressed = true; return 0;
        case WM_RBUTTONUP:   ev.rmb = false; return 0;
        case WM_MOUSEWHEEL: ev.wheel += GET_WHEEL_DELTA_WPARAM(wp) > 0 ? 1 : -1; return 0;
        case WM_MOUSEMOVE: {
            POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            if (g_grab) {
                if (g_haveLast) {
                    ev.dx += (float)(pt.x - g_centerX);
                    ev.dy += (float)(pt.y - g_centerY);
                }
                // вернуть курсор в центр
                RECT rc; GetClientRect(hwnd, &rc);
                POINT center{rc.right / 2, rc.bottom / 2};
                ClientToScreen(hwnd, &center);
                SetCursorPos(center.x, center.y);
                g_haveLast = true;
            } else {
                if (g_haveLast) { ev.dx += (float)(pt.x - g_lastMouse.x); ev.dy += (float)(pt.y - g_lastMouse.y); }
                g_lastMouse = pt; g_haveLast = true;
            }
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

bool Platform::initWindow(int w, int h, const char* title) {
    g_plat = this;
    static HINSTANCE hi = GetModuleHandle(nullptr);
    WNDCLASS wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hi;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "QuakelikeWnd";
    RegisterClass(&wc);
    DWORD style = WS_POPUP | WS_VISIBLE;
    HWND hwnd = CreateWindow(wc.lpszClassName, title, style,
                             (GetSystemMetrics(SM_CXSCREEN) - w) / 2,
                             (GetSystemMetrics(SM_CYSCREEN) - h) / 2,
                             w, h, nullptr, nullptr, hi, nullptr);
    if (!hwnd) return false;
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
    RECT rc; GetClientRect(hwnd, &rc);
    g_centerX = rc.right / 2; g_centerY = rc.bottom / 2;
    setMouseGrab(true);
    events.keyDown[K_ENTER] = false;
    return true;
}

void Platform::setMouseGrab(bool grab) {
    g_grab = grab;
    if (grab) ShowCursor(FALSE); else ShowCursor(TRUE);
}

void Platform::shutdown() {}

bool Platform::pumpEvents() {
    MSG m;
    while (PeekMessage(&m, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&m);
        DispatchMessage(&m);
    }
    if (events.quit) return false;
    return true;
}

void Platform::present(const uint32_t* fb, int w, int h) {
    HDC hdc = GetDC(GetForegroundWindow());
    if (!hdc) { ReleaseDC(GetForegroundWindow(), hdc); return; }
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(hdc, 0, 0, w, h, 0, 0, w, h, fb, &bmi, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(GetForegroundWindow(), hdc);
}

double getTimeSeconds() {
    static LARGE_INTEGER freq, start;
    static bool init = false;
    if (!init) { QueryPerformanceFrequency(&freq); QueryPerformanceCounter(&start); init = true; }
    LARGE_INTEGER now; QueryPerformanceCounter(&now);
    return (double)(now.QuadPart - start.QuadPart) / (double)freq.QuadPart;
}

void sleepMs(int ms) { Sleep(ms); }

#endif
