// map.cpp - генерация демо-арены в стиле Quake (e1m1-ish)
#include "map.h"

static const uint32_t C_WALL   = 0xff6b6b5a; // серо-коричневые стены
static const uint32_t C_FLOOR  = 0xff4a4a52;
static const uint32_t C_RUST   = 0xff8a5038;
static const uint32_t C_METAL  = 0xff5f7080;
static const uint32_t C_STAIR  = 0xff776655;

void GameMap::buildDemo() {
    brushes.clear();
    auto add = [&](float x0, float y0, float z0, float x1, float y1, float z1, uint32_t c) {
        brushes.push_back({{x0, y0, z0}, {x1, y1, z1}, c});
    };

    // Пол и потолок арены 40x40
    add(-20, -20, -0.5f, 20, 20, 0, C_FLOOR);
    add(-20, -20, 12, 20, 20, 12.5f, C_WALL);

    // Внешние стены
    add(-20.5f, -20.5f, 0, -20, 20.5f, 12, C_WALL);
    add(20, -20.5f, 0, 20.5f, 20.5f, 12, C_WALL);
    add(-20.5f, -20.5f, 0, 20.5f, -20, 12, C_WALL);
    add(-20.5f, 20, 0, 20.5f, 20.5f, 12, C_WALL);

    // Центральная площадка с пьедесталом
    add(-3, -3, 0, 3, 3, 0.6f, C_METAL);
    add(-1.2f, -1.2f, 0.6f, 1.2f, 1.2f, 1.4f, C_RUST);

    // Комната слева (с бронёй)
    add(-19, -8, 0, -10, -7.5f, 12, C_WALL);
    add(-19, 7.5f, 0, -10, 8, 12, C_WALL);
    add(-19, -8, 0, -18.5f, 8, 12, C_WALL);
    add(-10, -8, 0, -9.5f, -2, 12, C_WALL);
    add(-10, 2, 0, -9.5f, 8, 12, C_WALL);
    add(-10, -8, 8, -9.5f, 8, 12, C_WALL); // перемычка над входом

    // Комната справа (с ракетомётом)
    add(10, -8, 0, 19, -7.5f, 12, C_WALL);
    add(10, 7.5f, 0, 19, 8, 12, C_WALL);
    add(18.5f, -8, 0, 19, 8, 12, C_WALL);
    add(9.5f, -8, 0, 10, -2, 12, C_WALL);
    add(9.5f, 2, 0, 10, 8, 12, C_WALL);
    add(9.5f, -8, 8, 10, 8, 12, C_WALL);

    // Комната на севере (с плазмою) - балкон
    add(-8, 10, 0, -7.5f, 19, 12, C_WALL);
    add(7.5f, 10, 0, 8, 19, 12, C_WALL);
    add(-8, 18.5f, 0, 8, 19, 12, C_WALL);
    add(-8, 10, 0, 8, 10.5f, 4, C_WALL); // низкая передняя стенка
    // Ступени на балкон
    for (int i = 0; i < 8; i++) {
        float h = 0.f;
        // ступени вверх к северу
        add(-2 + i * 0.0f, 8.5f + i * 0.5f, 0, -2 + 4, 9 + i * 0.5f, 0.5f * (i + 1), C_STAIR);
        h = h;
    }

    // Колонны по углам центральной зоны
    add(-6, -6, 0, -5, -5, 12, C_RUST);
    add(5, -6, 0, 6, -5, 12, C_RUST);
    add(-6, 5, 0, -5, 6, 12, C_RUST);
    add(5, 5, 0, 6, 6, 12, C_RUST);

    // Ящики / укрытия
    add(3, -12, 0, 4.5f, -10.5f, 1.5f, C_METAL);
    add(-4.5f, 3, 0, -3, 4.5f, 1.5f, C_METAL);
    add(12, -4, 0, 13.5f, -2.5f, 1.5f, C_METAL);
    add(-13, 4, 0, -11.5f, 5.5f, 1.5f, C_METAL);
    add(0, -16, 0, 1.5f, -14.5f, 1.5f, C_METAL);

    // Южный коридор-спавн
    add(-4, -20, 0, -3.5f, -14, 12, C_WALL);
    add(3.5f, -20, 0, 4, -14, 12, C_WALL);
}

bool GameMap::boxOverlapsAny(const Vec3& min, const Vec3& max) const {
    for (const auto& b : brushes) {
        if (min.x < b.max.x && max.x > b.min.x &&
            min.y < b.max.y && max.y > b.min.y &&
            min.z < b.max.z && max.z > b.min.z)
            return true;
    }
    return false;
}

bool GameMap::clipHull(Vec3& pos, float radius, float height) const {
    bool clipped = false;
    const float half = height * 0.5f;
    for (int iter = 0; iter < 3; iter++) {
        Vec3 min = {pos.x - radius, pos.y - radius, pos.z - half};
        Vec3 max = {pos.x + radius, pos.y + radius, pos.z + half};
        bool hitThisIter = false;
        for (const auto& b : brushes) {
            if (!(min.x < b.max.x && max.x > b.min.x &&
                  min.y < b.max.y && max.y > b.min.y &&
                  min.z < b.max.z && max.z > b.min.z))
                continue;
            // выталкиваем по минимальной оси
            float oxp = b.max.x - min.x, oxm = max.x - b.min.x;
            float oyp = b.max.y - min.y, oym = max.y - b.min.y;
            float ozp = b.max.z - min.z, ozm = max.z - b.min.z;
            float ox = (oxp < oxm) ? -oxp : oxm;
            float oy = (oyp < oym) ? -oyp : oym;
            float oz = (ozp < ozm) ? -ozp : ozm;
            float m = std::fabs(ox), my = std::fabs(oy), mz = std::fabs(oz);
            if (my < m) { m = my; ox = 0; oy = (std::fabs(oyp) < std::fabs(oym)) ? -oyp : oym; }
            if (mz < m) { oz = (std::fabs(ozp) < std::fabs(ozm)) ? -ozp : ozm; ox = 0; oy = 0; }
            pos.x += ox; pos.y += oy; pos.z += oz;
            if (oz != 0) clipped = true;
            hitThisIter = true;
            clipped = true;
        }
        if (!hitThisIter) break;
    }
    return clipped;
}

static bool rayBox(const Vec3& o, const Vec3& d, const Vec3& mn, const Vec3& mx,
                   float maxT, float& tHit, Vec3& nOut) {
    float tmin = 0, tmax = maxT;
    int axisNeg = -1;
    for (int a = 0; a < 3; a++) {
        float oo = a == 0 ? o.x : a == 1 ? o.y : o.z;
        float dd = a == 0 ? d.x : a == 1 ? d.y : d.z;
        float m0 = a == 0 ? mn.x : a == 1 ? mn.y : mn.z;
        float m1 = a == 0 ? mx.x : a == 1 ? mx.y : mx.z;
        if (std::fabs(dd) < 1e-8f) {
            if (oo < m0 || oo > m1) return false;
        } else {
            float t0 = (m0 - oo) / dd;
            float t1 = (m1 - oo) / dd;
            int neg = 1;
            if (t0 > t1) { std::swap(t0, t1); neg = -1; }
            if (t0 > tmin) { tmin = t0; axisNeg = a * 2 + (neg > 0 ? 0 : 1); }
            if (t1 < tmax) tmax = t1;
            if (tmin > tmax) return false;
        }
    }
    if (tmin <= 0 || tmin >= maxT) return false;
    tHit = tmin;
    nOut = {0, 0, 0};
    if (axisNeg >= 0) {
        int a = axisNeg / 2;
        float s = (axisNeg % 2 == 0) ? -1.f : 1.f;
        if (a == 0) nOut = {s, 0, 0};
        if (a == 1) nOut = {0, s, 0};
        if (a == 2) nOut = {0, 0, s};
    }
    return true;
}

float GameMap::rayCast(const Vec3& o, const Vec3& d, float maxT, Vec3* normal) const {
    float best = -1;
    Vec3 bn;
    for (const auto& b : brushes) {
        float t;
        Vec3 n;
        if (rayBox(o, d, b.min, b.max, maxT, t, n)) {
            if (best < 0 || t < best) { best = t; bn = n; }
        }
    }
    if (normal && best >= 0) *normal = bn;
    return best;
}

bool GameMap::rayHitsBrush(const Vec3& o, const Vec3& d, float t, float dt) const {
    Vec3 p = o + d * dt;
    for (const auto& b : brushes) {
        if (p.x > b.min.x && p.x < b.max.x &&
            p.y > b.min.y && p.y < b.max.y &&
            p.z > b.min.z && p.z < b.max.z)
            return true;
    }
    (void)o; (void)t;
    return false;
}
