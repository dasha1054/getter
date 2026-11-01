// renderer.cpp - raycast рендер: стены из brush-ов, спрайты, HUD в стиле Quake
#include "renderer.h"
#include <cmath>
#include <cstring>
#include <algorithm>

static inline uint32_t shade(uint32_t rgba, float f) {
    int a = (rgba >> 24) & 0xff;
    int r = (int)(((rgba >> 16) & 0xff) * f);
    int g = (int)(((rgba >> 8) & 0xff) * f);
    int b = (int)((rgba & 0xff) * f);
    r = std::min(255, r); g = std::min(255, g); b = std::min(255, b);
    return (a << 24) | (r << 16) | (g << 8) | b;
}

void Renderer::init(int w, int h) {
    width = w; height = h;
    framebuffer.assign(w * h, 0xff101018);
    zbuf.assign(w * h, 1e9f);
}

void Renderer::setupCamera(const Game& g) {
    const PlayerState& p = g.player;
    cam.pos = p.pos + Vec3{0, 0, Game::PLAYER_HEIGHT * 0.5f - 0.1f};
    // bob & kick
    float hs = std::sqrt(p.vel.x * p.vel.x + p.vel.y * p.vel.y);
    if (p.onGround && hs > 0.5f) {
        cam.pos.z += std::sin(p.bobPhase * 2.f) * 0.03f;
        cam.pos += Vec3{-std::sin(p.yaw), std::cos(p.yaw), 0} * (std::sin(p.bobPhase) * 0.02f);
    }
    float yaw = p.yaw;
    float pitch = p.pitch - p.viewKick * 0.03f;
    cam.fwd = dirFromAngles(yaw, pitch);
    cam.right = {std::cos(yaw - 1.5707963f), std::sin(yaw - 1.5707963f), 0};
    cam.up = cam.fwd.cross(cam.right).normalized();
    cam.tanHalfFov = std::tan(p.fov * 0.5f * 3.14159265f / 180.f);
    cam.aspect = (float)width / (float)height;
    cam.tanHalfFovY = cam.tanHalfFov / cam.aspect;
}

bool Renderer::project(const Vec3& wp, float& sx, float& sy, float& depth) const {
    Vec3 d = wp - cam.pos;
    float zc = d.dot(cam.fwd);
    if (zc < 0.05f) return false;
    float xc = d.dot(cam.right);
    float yc = d.dot(cam.up);
    float ndcX = (xc / zc) / cam.tanHalfFov;
    float ndcY = (yc / zc) / cam.tanHalfFovY;
    sx = (ndcX * 0.5f + 0.5f) * width;
    sy = (0.5f - ndcY * 0.5f) * height;
    depth = zc;
    return true;
}

void Renderer::fillRect(int x, int y, int w, int h, uint32_t color) {
    for (int j = std::max(0, y); j < std::min(height, y + h); j++)
        for (int i = std::max(0, x); i < std::min(width, x + w); i++)
            framebuffer[j * width + i] = color;
}

void Renderer::blendRect(int x, int y, int w, int h, uint32_t color, float alpha) {
    for (int j = std::max(0, y); j < std::min(height, y + h); j++)
        for (int i = std::max(0, x); i < std::min(width, x + w); i++) {
            uint32_t dst = framebuffer[j * width + i];
            int r = (int)((((color >> 16) & 0xff) * alpha) + ((dst >> 16) & 0xff) * (1 - alpha));
            int g = (int)((((color >> 8) & 0xff) * alpha) + ((dst >> 8) & 0xff) * (1 - alpha));
            int b = (int)(((color & 0xff) * alpha) + (dst & 0xff) * (1 - alpha));
            framebuffer[j * width + i] = (0xff << 24) | (r << 16) | (g << 8) | b;
        }
}

void Renderer::drawLine(float x0, float y0, float x1, float y1, uint32_t color, float widthPx) {
    // растеризация толстой линии в экранном пространстве с z-тестом по середине
    float len = std::hypot(x1 - x0, y1 - y0);
    if (len < 0.001f) return;
    int steps = (int)(len * 2) + 1;
    float hw = widthPx * 0.5f;
    int wi = std::max(1, (int)hw);
    for (int s = 0; s <= steps; s++) {
        float t = (float)s / steps;
        int px = (int)(x0 + (x1 - x0) * t);
        int py = (int)(y0 + (y1 - y0) * t);
        for (int oy = -wi; oy <= wi; oy++)
            for (int ox = -wi; ox <= wi; ox++) {
                int ix = px + ox, iy = py + oy;
                if (ix >= 0 && ix < width && iy >= 0 && iy < height)
                    framebuffer[iy * width + ix] = color;
            }
    }
}

void Renderer::drawCircleSprite(const Vec3& center, float worldRadius, uint32_t color, float alpha) {
    float sx, sy, depth;
    if (!project(center, sx, sy, depth)) return;
    float radiusPx = (worldRadius / depth) / cam.tanHalfFovY * height * 0.5f;
    int r = (int)radiusPx;
    for (int j = -r; j <= r; j++) {
        int iy = (int)sy + j;
        if (iy < 0 || iy >= height) continue;
        int half = (int)std::sqrt(std::max(0.f, (float)(r * r - j * j)));
        for (int i = -half; i <= half; i++) {
            int ix = (int)sx + i;
            if (ix < 0 || ix >= width) continue;
            int idx = iy * width + ix;
            if (depth >= zbuf[idx]) continue;
            uint32_t c = shade(color, 1.f - std::fabs(j) / (r + 1.f) * 0.4f);
            if (alpha >= 0.999f) framebuffer[idx] = c;
            else {
                uint32_t dst = framebuffer[idx];
                int rr = (int)((((c >> 16) & 0xff) * alpha) + ((dst >> 16) & 0xff) * (1 - alpha));
                int gg = (int)((((c >> 8) & 0xff) * alpha) + ((dst >> 8) & 0xff) * (1 - alpha));
                int bb = (int)(((c & 0xff) * alpha) + (dst & 0xff) * (1 - alpha));
                framebuffer[idx] = (0xff << 24) | (rr << 16) | (gg << 8) | bb;
            }
        }
    }
}

// ---- растеризация triangle/quad faces of brushes ----
struct ScreenVert { float x, y, z; };

static void putPix(std::vector<uint32_t>& fb, std::vector<float>& zb, int w, int h,
                   int x, int y, float z, uint32_t color) {
    if (x < 0 || x >= w || y < 0 || y >= h) return;
    int idx = y * w + x;
    if (z < zb[idx]) { zb[idx] = z; fb[idx] = color; }
}

static void drawTriangleFB(std::vector<uint32_t>& fb, std::vector<float>& zb, int w, int h,
                           const ScreenVert& v0, const ScreenVert& v1, const ScreenVert& v2,
                           uint32_t color) {
    float minX = std::min({v0.x, v1.x, v2.x});
    float maxX = std::max({v0.x, v1.x, v2.x});
    float minY = std::min({v0.y, v1.y, v2.y});
    float maxY = std::max({v0.y, v1.y, v2.y});
    int ix0 = std::max(0, (int)std::floor(minX));
    int ix1 = std::min(w - 1, (int)std::ceil(maxX));
    int iy0 = std::max(0, (int)std::floor(minY));
    int iy1 = std::min(h - 1, (int)std::ceil(maxY));
    float denom = (v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y);
    if (std::fabs(denom) < 1e-9f) return;
    float invD = 1.f / denom;
    for (int y = iy0; y <= iy1; y++) {
        for (int x = ix0; x <= ix1; x++) {
            float px = x + 0.5f, py = y + 0.5f;
            float a = ((v1.y - v2.y) * (px - v2.x) + (v2.x - v1.x) * (py - v2.y)) * invD;
            float b = ((v2.y - v0.y) * (px - v2.x) + (v0.x - v2.x) * (py - v2.y)) * invD;
            float c = 1.f - a - b;
            if (a < -0.001f || b < -0.001f || c < -0.001f) continue;
            float z = a * v0.z + b * v1.z + c * v2.z;
            putPix(fb, zb, w, h, x, y, z, color);
        }
    }
}

void Renderer::drawWorld(const Game& g) {
    // clear
    uint32_t sky = 0xff0a0a12;
    for (auto& p : framebuffer) p = sky;
    std::fill(zbuf.begin(), zbuf.end(), 1e9f);

    struct ProjFace {
        ScreenVert v[4];
        int n;
        uint32_t color;
        float depth;
    };
    std::vector<ProjFace> faces;
    faces.reserve((g.map.brushes.size() + g.map.doors.size()) * 6);

    const auto& brushes = g.map.brushes;
    // brush-ы + двери как единый набор AABB (двери — со смещением curOffset)
    auto emitBox = [&](const Vec3& bmin, const Vec3& bmax, uint32_t bcolor) {
        Vec3 camToC = (bmin + bmax) * 0.5f - cam.pos;
        float distC = camToC.length();
        if (distC > 70.f) return;

        // грани: для каждой оси - две стороны
        for (int axis = 0; axis < 3; axis++) {
            for (int side = 0; side < 2; side++) {
                // нормаль
                Vec3 n = {0, 0, 0};
                if (axis == 0) n.x = side ? 1 : -1;
                if (axis == 1) n.y = side ? 1 : -1;
                if (axis == 2) n.z = side ? 1 : -1;
                if (n.dot(-camToC) <= 0.f) continue; // не видна

                // corners of face
                Vec3 corners[4];
                float lo = axis == 0 ? bmin.x : axis == 1 ? bmin.y : bmin.z;
                float hi = axis == 0 ? bmax.x : axis == 1 ? bmax.y : bmax.z;
                float val = side ? hi : lo;
                int u1 = (axis + 1) % 3, u2 = (axis + 2) % 3;
                auto setComp = [](Vec3& v, int ax, float val) {
                    if (ax == 0) v.x = val; else if (ax == 1) v.y = val; else v.z = val;
                };
                float l1 = u1 == 0 ? bmin.x : u1 == 1 ? bmin.y : bmin.z;
                float h1 = u1 == 0 ? bmax.x : u1 == 1 ? bmax.y : bmax.z;
                float l2 = u2 == 0 ? bmin.x : u2 == 1 ? bmin.y : bmin.z;
                float h2 = u2 == 0 ? bmax.x : u2 == 1 ? bmax.y : bmax.z;
                corners[0] = bmin; setComp(corners[0], axis, val); setComp(corners[0], u1, l1); setComp(corners[0], u2, l2);
                corners[1] = bmin; setComp(corners[1], axis, val); setComp(corners[1], u1, h1); setComp(corners[1], u2, l2);
                corners[2] = bmin; setComp(corners[2], axis, val); setComp(corners[2], u1, h1); setComp(corners[2], u2, h2);
                corners[3] = bmin; setComp(corners[3], axis, val); setComp(corners[3], u1, l1); setComp(corners[3], u2, h2);

                // simple fog by distance
                float fog = std::max(0.25f, 1.f - distC / 55.f);
                // directional light fake: top brighter
                float light = fog * (axis == 2 && side ? 1.15f : (axis == 2 ? 0.55f : 0.8f));
                uint32_t col = shade(bcolor, light);

                ProjFace pf; pf.n = 4; pf.color = col; pf.depth = distC;
                bool ok = true;
                for (int i = 0; i < 4; i++) {
                    float sx, sy, dz;
                    if (!project(corners[i], sx, sy, dz)) { ok = false; break; }
                    pf.v[i] = {sx, sy, dz};
                }
                if (ok) faces.push_back(pf);
            }
        }
    };
    for (const auto& b : brushes) emitBox(b.min, b.max, b.color);
    for (const auto& d : g.map.doors) {
        if (d.curOffset >= d.travel - 0.01f) continue;
        Vec3 mn = d.closedMin, mx = d.closedMax;
        mn.z += d.curOffset; mx.z += d.curOffset;
        emitBox(mn, mx, d.color);
    }

    // painter's algorithm: sort far->near
    std::sort(faces.begin(), faces.end(), [](const ProjFace& a, const ProjFace& b) {
        return a.depth > b.depth;
    });

    for (const auto& f : faces) {
        drawTriangleFB(framebuffer, zbuf, width, height, f.v[0], f.v[1], f.v[2], f.color);
        drawTriangleFB(framebuffer, zbuf, width, height, f.v[0], f.v[2], f.v[3], f.color);
    }

    // трассеры
    for (const auto& t : g.tracers) {
        float x0, y0, d0, x1, y1, d1;
        bool a = project(t.a, x0, y0, d0);
        bool b2 = project(t.b, x1, y1, d1);
        if (a && b2) drawLine(x0, y0, x1, y1, t.color, 1.5f);
    }
}

void Renderer::drawBillboards(const Game& g) {
    // враги - billboards (как спрайты монстров в quake)
    struct Item { float depth; int kind; int index; };
    std::vector<Item> items;
    for (size_t i = 0; i < g.enemies.size(); i++) {
        const Enemy& e = g.enemies[i];
        Vec3 d = e.pos - cam.pos;
        float zc = d.dot(cam.fwd);
        if (zc < 0.2f) continue;
        items.push_back({zc, 0, (int)i});
    }
    for (size_t i = 0; i < g.pickups.size(); i++) {
        const Pickup& pk = g.pickups[i];
        if (pk.taken) continue;
        Vec3 d = pk.pos - cam.pos;
        float zc = d.dot(cam.fwd);
        if (zc < 0.2f) continue;
        items.push_back({zc, 1, (int)i});
    }
    for (size_t i = 0; i < g.projectiles.size(); i++) {
        Vec3 d = g.projectiles[i].pos - cam.pos;
        float zc = d.dot(cam.fwd);
        if (zc < 0.2f) continue;
        items.push_back({zc, 2, (int)i});
    }
    for (size_t i = 0; i < g.explosions.size(); i++) {
        Vec3 d = g.explosions[i].pos - cam.pos;
        float zc = d.dot(cam.fwd);
        if (zc < 0.2f) continue;
        items.push_back({zc, 3, (int)i});
    }
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
        return a.depth > b.depth;
    });

    for (const auto& it : items) {
        if (it.kind == 0) {
            const Enemy& e = g.enemies[it.index];
            if (!e.alive) continue;
            // billboard quad: up = cam.up, right = cam.right
            Vec3 c = e.pos + Vec3{0, 0, 0};
            float hh = e.height * 0.5f;
            Vec3 top = c + cam.up * hh * 1.0f + Vec3{0, 0, hh * 0.9f};
            Vec3 bot = c - Vec3{0, 0, hh};
            Vec3 lr = cam.right * e.radius;
            // рисуем как вертикальный прямоугольник через project 4 углов
            float tx0, ty0, td;
            if (!project(top, tx0, ty0, td)) continue;
            float bx0, by0, bd;
            if (!project(bot, bx0, by0, bd)) continue;
            float mx, my, md;
            if (!project(c, mx, my, md)) continue;
            float edgeX, edgeY, ed;
            if (!project(c + lr, edgeX, edgeY, ed)) continue;
            float halfW = std::fabs(edgeX - mx);
            uint32_t col = e.color;
            float fog = std::max(0.3f, 1.f - md / 50.f);
            col = shade(col, fog);
            if (e.painFlash > 0) col = 0xffffd0d0;
            int x0 = (int)(mx - halfW), x1 = (int)(mx + halfW);
            int yTop = (int)std::min(ty0, by0), yBot = (int)std::max(ty0, by0);
            // z-test по центру
            int ci = (int)mx; if (ci < 0) ci = 0; if (ci >= width) ci = width - 1;
            int cj = (int)((yTop + yBot) / 2); if (cj < 0) cj = 0; if (cj >= height) cj = height - 1;
            if (md < zbuf[cj * width + ci]) {
                fillRect(x0, yTop, x1 - x0, yBot - yTop, col);
                // "лицо": глаза
                blendRect(x0 + (x1 - x0) / 4, yTop + (yBot - yTop) / 5, (x1 - x0) / 8, (yBot - yTop) / 12, 0xff000000, 0.8f);
                blendRect(x1 - (x1 - x0) / 4 - (x1 - x0) / 8, yTop + (yBot - yTop) / 5, (x1 - x0) / 8, (yBot - yTop) / 12, 0xff000000, 0.8f);
            }
        } else if (it.kind == 1) {
            const Pickup& pk = g.pickups[it.index];
            static const uint32_t colors[] = {
                0xff40ff40, 0xff40ff40, 0xffffff40, 0xffffff40, 0xff40c0ff,
                0xffc0c0c0, 0xffff8040, 0xffc0c0c0, 0xff60c0ff, 0xffffc040,
                0xffff4040, 0xff00ffff, 0xffd0d0e0 // PICK_KEY1 silver, PICK_KEY2 gold
            };
            Vec3 pos = pk.pos + Vec3(0.f, 0.f, std::sin(g.time * 3.f + pk.bobPhase) * 0.15f);
            uint32_t col = colors[pk.type % 13];
            if (pk.type == PICK_QUAD) col = 0xff4060ff;
            if (pk.type == PICK_INVULN) col = 0xffff00ff;
            if (pk.type == PICK_KEY1) col = 0xffd8d8e8;
            if (pk.type == PICK_KEY2) col = 0xffffd040;
            float rad = (pk.type == PICK_KEY1 || pk.type == PICK_KEY2) ? 0.25f : 0.35f;
            drawCircleSprite(pos, rad, col, 0.95f);
            drawCircleSprite(pos, rad + 0.2f, col, 0.25f);
        } else if (it.kind == 2) {
            const Projectile& pr = g.projectiles[it.index];
            drawCircleSprite(pr.pos, pr.radius, pr.color, 1.f);
            drawCircleSprite(pr.pos, pr.radius * 2.f, pr.color, 0.3f);
        } else {
            const Explosion& ex = g.explosions[it.index];
            float f = ex.t / 0.35f;
            float r = ex.radius * (0.4f + f * 0.9f);
            uint32_t col = f < 0.4f ? 0xfffff0a0 : 0xffff8030;
            drawCircleSprite(ex.pos, r, col, std::max(0.f, 1.f - f));
            drawCircleSprite(ex.pos, r * 1.4f, 0xff402000, std::max(0.f, 0.7f - f));
        }
    }
}


void Renderer::drawGun(const Game& g) {
    // псевдо weapon-view: простая форма в зависимости от оружия
    int w = g.hud.currentWeapon;
    float kick = g.player.viewKick;
    float cx = width * 0.68f;
    float cy = height * (0.86f + kick * 0.06f);
    uint32_t metal = 0xff3a3a42;
    uint32_t wood = 0xff6a4a2a;
    switch (w) {
        case W_MELEE: {
            // кулак
            float fx = cx + 60 * std::sin(kick * 3.14f);
            drawLine(fx, cy + 80, fx - 30, cy - 20, 0xffc8a078, 26);
            break;
        }
        case W_SHOTGUN:
            drawLine(cx, cy + 90, cx - 10, cy - 40, wood, 14);
            drawLine(cx - 10, cy - 40, cx - 14, cy - 90, metal, 10);
            break;
        case W_SSG:
            drawLine(cx, cy + 90, cx - 10, cy - 40, wood, 16);
            drawLine(cx - 10, cy - 40, cx - 16, cy - 95, metal, 8);
            drawLine(cx - 2, cy - 40, cx + 2, cy - 95, metal, 8);
            break;
        case W_MG:
            drawLine(cx, cy + 90, cx - 8, cy - 30, metal, 16);
            drawLine(cx - 8, cy - 30, cx - 12, cy - 95, 0xff2a2a2a, 9);
            break;
        case W_WS:
            drawLine(cx, cy + 90, cx - 10, cy - 40, 0xff4a5a6a, 14);
            drawLine(cx - 10, cy - 40, cx - 14, cy - 85, metal, 12);
            break;
        case W_SUPER_NAIL:
            drawLine(cx, cy + 90, cx - 10, cy - 40, 0xff4a5a6a, 18);
            drawLine(cx - 10, cy - 40, cx - 16, cy - 95, metal, 16);
            break;
        case W_ROCKET:
            drawLine(cx, cy + 90, cx - 6, cy - 20, metal, 22);
            drawLine(cx - 6, cy - 20, cx - 10, cy - 80, 0xff202028, 20);
            break;
        case W_PLASMA:
            drawLine(cx, cy + 90, cx - 10, cy - 40, 0xff304050, 16);
            drawLine(cx - 10, cy - 40, cx - 14, cy - 80, 0xff60c0ff, 10);
            break;
    }
    // вспышка выстрела
    if (kick > 0.3f) {
        int fx = (int)(cx - 14), fy = (int)(cy - 90);
        blendRect(fx - 18, fy - 18, 36, 36, 0xfffff0a0, kick * 0.8f);
        blendRect(fx - 30, fy - 30, 60, 60, 0xffffc060, kick * 0.35f);
    }
}

// --- минимальный 5x7 bitmap font (A-Z 0-9 базовые символы) ---
// encoded rows as bitmasks per char
struct Glyph { char c; uint8_t rows[7]; };
static const Glyph kFont[] = {
#include "font_data.h"
};
static const int kFontCount = sizeof(kFont) / sizeof(Glyph);

void Renderer::drawText(int x, int y, const char* text, uint32_t color, int scale) {
    int penX = x;
    for (const char* ch = text; *ch; ch++) {
        char c = *ch;
        if (c >= 'a' && c <= 'z') c -= 32;
        if (c == ' ') { penX += 6 * scale; continue; }
        const Glyph* gl = nullptr;
        for (int i = 0; i < kFontCount; i++)
            if (kFont[i].c == c) { gl = &kFont[i]; break; }
        if (!gl) { penX += 6 * scale; continue; }
        for (int row = 0; row < 7; row++) {
            uint8_t bits = gl->rows[row];
            for (int col = 0; col < 5; col++) {
                if (bits & (1 << (4 - col))) {
                    fillRect(penX + col * scale, y + row * scale, scale, scale, color);
                }
            }
        }
        penX += 6 * scale;
    }
}

void Renderer::drawHud(const Game& g) {
    const HudState& hud = g.hud;
    int S = std::max(2, height / 240);
    int baseY = height - 8 * S - 2;

    // нижняя панель
    blendRect(0, baseY - 2, width, height - baseY + 4, 0xff202028, 0.75f);

    char buf[64];
    // health
    snprintf(buf, sizeof(buf), "%d", (int)hud.health);
    uint32_t hc = hud.health > 25 ? 0xffff4040 : 0xffff0000;
    drawText(S * 2, baseY, "MEDIC", 0xffa0a0a0, S);
    drawText(S * 2, baseY + S * 8, buf, hc, S * 2);

    // armor
    if (hud.armor > 0) {
        snprintf(buf, sizeof(buf), "%d", (int)hud.armor);
        drawText(S * 14, baseY, "ARMOR", 0xffa0a0a0, S);
        drawText(S * 14, baseY + S * 8, buf, 0xffffff40, S * 2);
    }

    // ammo
    int ai = 0;
    switch (hud.currentWeapon) {
        case W_SHOTGUN: case W_SSG: ai = 1; break;
        case W_WS: case W_SUPER_NAIL: ai = 2; break;
        case W_ROCKET: ai = 3; break;
        case W_PLASMA: ai = 4; break;
        default: ai = 0; break;
    }
    if (hud.currentWeapon != W_MELEE) {
        snprintf(buf, sizeof(buf), "%d", hud.ammo[ai]);
        drawText(width - S * 20, baseY, "AMMO", 0xffa0a0a0, S);
        drawText(width - S * 20, baseY + S * 8, buf, 0xffc0c040, S * 2);
    }

    // оружие справа списком
    int wy = baseY - S * 10 * 8;
    for (int i = 0; i < W_COUNT; i++) {
        if (!hud.weaponsOwned[i]) continue;
        bool cur = (i == hud.currentWeapon);
        snprintf(buf, sizeof(buf), "%d %s", i + 1, kWeapons[i].shortName);
        drawText(width - S * 24, wy, buf, cur ? 0xffffff40 : 0xff808080, S);
        wy += S * 10;
    }

    // kills / time
    snprintf(buf, sizeof(buf), "KILLS %d", hud.kills);
    drawText(S * 2, S * 2, buf, 0xffffffff, S);
    snprintf(buf, sizeof(buf), "TIME %d", (int)hud.timeAlive);
    drawText(S * 2, S * 12, buf, 0xffa0a0a0, S);
    snprintf(buf, sizeof(buf), "%s", g.map.name);
    drawText(width / 2 - (int)strlen(g.map.name) * 3 * S / 2, S * 2, buf, 0xffc0a060, S);
    if (hud.keys[1] > 0) drawText(S * 2, S * 22, "SILVER KEY", 0xffd8d8e8, S);
    if (hud.keys[2] > 0) drawText(S * 2, S * 34, "GOLD KEY", 0xffffd040, S);

    // подсказка USE рядом с дверью
    for (const auto& d : g.map.doors) {
        if (d.open || d.animT >= 0.f) continue;
        float dx = std::fabs((d.closedMin.x + d.closedMax.x) * 0.5f - g.player.pos.x);
        float dy = std::fabs((d.closedMin.y + d.closedMax.y) * 0.5f - g.player.pos.y);
        float dz = std::fabs((d.closedMin.z + d.closedMax.z) * 0.5f - g.player.pos.z);
        if (dx < 4.f && dy < 4.f && dz < 6.f) {
            const char* hint = d.keyId == 2 ? "PRESS E - GOLD DOOR" :
                               d.keyId == 1 ? "PRESS E - SILVER DOOR" : "PRESS E - OPEN";
            drawText(width / 2 - (int)strlen(hint) * 3 * S / 2, height / 2 + S * 14, hint, 0xffffff80, S);
            break;
        }
    }

    // quad / invuln
    if (hud.quadTime > 0) {
        snprintf(buf, sizeof(buf), "QUAD %d", (int)hud.quadTime);
        drawText(width / 2 - S * 24, S * 2, buf, 0xff4060ff, S * 2);
    }
    if (hud.invulnTime > 0) {
        snprintf(buf, sizeof(buf), "INVULN %d", (int)hud.invulnTime);
        drawText(width / 2 - S * 30, S * 12, buf, 0xffff00ff, S * 2);
    }

    // сообщение
    if (g.messageTime > 0 && !g.message.empty()) {
        int len = (int)g.message.size();
        drawText(width / 2 - len * 3 * S / 2, height / 3, g.message.c_str(), 0xffffff40, S);
    }

    // damage flash
    if (hud.damageFlash > 0)
        blendRect(0, 0, width, height, 0xffcc0000, hud.damageFlash * 0.45f);
    // quad tint
    if (hud.quadTime > 0)
        blendRect(0, 0, width, height, 0xff2020aa, 0.12f);
    // invuln tint
    if (hud.invulnTime > 0)
        blendRect(0, 0, width, height, 0xffaa20aa, 0.12f);

    // speed lines при быстром движении (quake fast-FW feel)
    if (g.player.speedLines > 0.55f && !g.player.onGround) {
        float a = (g.player.speedLines - 0.55f) * 0.8f;
        for (int i = 0; i < 12; i++) {
            float ang = i * 0.5236f;
            float r0 = width * 0.35f;
            float x0 = width / 2 + std::cos(ang) * r0;
            float y0 = height / 2 + std::sin(ang) * r0;
            float x1 = width / 2 + std::cos(ang) * width;
            float y1 = height / 2 + std::sin(ang) * height;
            (void)x0; (void)y0; (void)x1; (void)y1;
            blendRect((int)x0, (int)y0, 2, 2, 0xffffffff, a * 0.3f);
            blendRect((int)x1, (int)y1, 2, 2, 0xffffffff, a * 0.3f);
        }
    }

    // crosshair
    int cx = width / 2, cy = height / 2;
    uint32_t ch = hud.quadTime > 0 ? 0xff4060ff : 0xff40ff40;
    fillRect(cx - S * 3, cy, S * 2, S, ch);
    fillRect(cx + S, cy, S * 2, S, ch);
    fillRect(cx, cy - S * 3, S, S * 2, ch);
    fillRect(cx, cy + S, S, S * 2, ch);
    fillRect(cx, cy, S, S, ch);

    if (hud.dead) {
        blendRect(0, 0, width, height, 0xff800000, 0.5f);
        drawText(width / 2 - 20 * S, height / 2, "YOU DIED", 0xffff4040, S * 3);
    }
}

void Renderer::render(const Game& g) {
    setupCamera(g);
    drawWorld(g);
    drawBillboards(g);
    if (g.started) {
        drawGun(g);
        drawHud(g);
    }
}
