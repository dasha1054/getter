// renderer.h - программный raycast-рендер в стиле Quake (BSP-like), без внешних библиотек
#pragma once
#include "game.h"
#include <cstdint>
#include <vector>
#include <string>

class Renderer {
public:
    int width = 0, height = 0;
    std::vector<uint32_t> framebuffer;   // XRGB8888
    std::vector<float> zbuf;

    void init(int w, int h);
    void render(const Game& g);

    // простой растровый шрифт/text
    void drawText(int x, int y, const char* text, uint32_t color, int scale = 1);

private:
    struct CamSpace {
        Vec3 pos, fwd, right, up;
        float tanHalfFov, aspect, tanHalfFovY;
    };
    CamSpace cam;

    void setupCamera(const Game& g);
    void drawWorld(const Game& g);
    void drawBillboards(const Game& g);
    void drawHud(const Game& g);
    void drawGun(const Game& g);

    // проекция точки камеры -> экран. false если позади near plane
    bool project(const Vec3& p, float& sx, float& sy, float& depth) const;
    void drawLine(float x0, float y0, float x1, float y1, uint32_t color, float widthPx);
    void fillRect(int x, int y, int w, int h, uint32_t color);
    void blendRect(int x, int y, int w, int h, uint32_t color, float alpha);
    void drawCircleSprite(const Vec3& center, float worldRadius, uint32_t color, float alpha);
};
