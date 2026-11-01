// map.cpp - генерация уровней в стиле Quake (e1m1-ish + e1m2-ish dungeon)
#include "map.h"

static const uint32_t C_WALL   = 0xff6b6b5a; // серо-коричневые стены
static const uint32_t C_FLOOR  = 0xff4a4a52;
static const uint32_t C_RUST   = 0xff8a5038;
static const uint32_t C_METAL  = 0xff5f7080;
static const uint32_t C_STAIR  = 0xff776655;
static const uint32_t C_DOOR   = 0xff9a7a3a; // двери — бронзовые, как в quake
static const uint32_t C_DWALL  = 0xff50465a; // тёмный камень темницы
static const uint32_t C_DFLOOR = 0xff3a3540;
static const uint32_t C_LAVA   = 0xffc03a10;

void GameMap::buildLevel(int num) {
    doors.clear();
    if (num == 2) { name = "E1M2 - THE DUNGEON"; buildDungeon(); return; }
    name = "E1M1 - DEAD SIMPLE";
    buildDemo();
}

void GameMap::buildDungeon() {
    brushes.clear();
    auto add = [&](float x0, float y0, float z0, float x1, float y1, float z1, uint32_t c) {
        brushes.push_back({{x0, y0, z0}, {x1, y1, z1}, c});
    };
    auto addDoor = [&](float x0, float y0, float z0, float x1, float y1, float z1, int keyId) {
        Door d;
        d.closedMin = {x0, y0, z0};
        d.closedMax = {x1, y1, z1};
        d.color = C_DOOR;
        d.keyId = keyId;
        d.travel = z1 - z0;
        d.forLevel1 = false;
        doors.push_back(d);
    };

    // Арена 44x36: вход на юге -> коридор с дверью (нужен silver key) -> тронный зал
    add(-22, -18, -0.5f, 22, 18, 0, C_DFLOOR);          // пол
    add(-22, -18, 11, 22, 18, 11.5f, C_DWALL);          // потолок
    // внешние стены
    add(-22.5f, -18.5f, 0, -22, 18.5f, 11, C_DWALL);
    add(22, -18.5f, 0, 22.5f, 18.5f, 11, C_DWALL);
    add(-22.5f, -18.5f, 0, 22.5f, -18, C_DWALL);
    add(-22.5f, 18, 0, 22.5f, 18.5f, 11, C_DWALL);

    // Стартовая комната (юг), разделена стеной с дверным проёмом по y=-6
    add(-22, -6, 0, -3.5f, -5.5f, 11, C_DWALL);
    add(3.5f, -6, 0, 22, -5.5f, 11, C_DWALL);
    add(-22, -6, 8, -3.5f, -5.5f, 11, C_DWALL); // перемычка над дверью
    add(3.5f, -6, 8, 22, -5.5f, 11, C_DWALL);
    addDoor(-3.5f, -6, 0, 3.5f, -5.5f, 8, 1);   // СЕРЕБРИСТАЯ ДВЕРЬ (нужен silver key)

    // Серебряная дверь №2: из среднего зала в тронный зал (y=+6)
    add(-22, 6, 0, -3.5f, 6.5f, 11, C_DWALL);
    add(3.5f, 6, 0, 22, 6.5f, 11, C_DWALL);
    add(-22, 6, 8, -3.5f, 6.5f, 11, C_DWALL);
    add(3.5f, 6, 8, 22, 6.5f, 11, C_DWALL);
    addDoor(-3.5f, 6, 0, 3.5f, 6.5f, 8, 2);     // ЗОЛОТАЯ ДВЕРЬ (нужен gold key)

    // Колонны в среднем зале
    for (int i = -1; i <= 1; i += 2)
        for (int j = -1; j <= 1; j += 2)
            add(i * 8 - 0.6f, j * 3 - 0.6f, 0, i * 8 + 0.6f, j * 3 + 0.6f, 11, C_RUST);

    // Ямы с лавой в среднем зале (не проходимо — просто свечение пола)
    add(-6, -2.5f, -0.49f, -2, -1.5f, -0.4f, C_LAVA);
    add(2, -2.5f, -0.49f, 6, -1.5f, -0.4f, C_LAVA);

    // Ступени к северной нише (серебряный ключ там)
    for (int i = 0; i < 6; i++)
        add(-2, 10 + i * 0.6f, 0, 2, 10.6f + i * 0.6f, 0.45f * (i + 1), C_STAIR);
    add(-4, 13.5f, 0, 4, 17.5f, 2.7f, C_METAL); // площадка ниши

    // Тронный зал (север за золотой дверью): возвышение + трон
    add(-10, 12, 0, 10, 12.5f, 11, C_DWALL);
    add(-10, 12, 0, -9.5f, 18, 11, C_DWALL);
    add(9.5f, 12, 0, 10, 18, 11, C_DWALL);
    add(-6, 13, 0, 6, 14, 0.8f, C_STAIR);
    add(-5, 14, 0.8f, 5, 17, 1.6f, C_METAL);
    add(-2, 15, 1.6f, 2, 16.5f, 3.2f, C_RUST); // трон

    // Боковые камеры (за решётками) — запад/восток стартовой комнаты
    add(-22, -17, 0, -14, -16.5f, 11, C_DWALL);
    add(-14, -17, 0, -13.5f, -10, 11, C_DWALL);
    add(-22, -10.5f, 0, -14, -10, 11, C_DWALL);
    add(-14, -17, 8, -13.5f, -10, 11, C_DWALL); // перемычка
    add(14, -17, 0, 22, -16.5f, 11, C_DWALL);
    add(13.5f, -17, 0, 14, -10, 11, C_DWALL);
    add(13.5f, -10.5f, 0, 22, -10, 11, C_DWALL);
    add(13.5f, -17, 8, 14, -10, 11, C_DWALL);

    // Ящики-укрытия
    add(-10, -14, 0, -8.5f, -12.5f, 1.5f, C_METAL);
    add(8, -13, 0, 9.5f, -11.5f, 1.5f, C_METAL);
    add(16, -2, 0, 17.5f, -0.5f, 1.5f, C_METAL);
    add(-17, -1, 0, -15.5f, 0.5f, 1.5f, C_METAL);
}

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
        auto testBox = [&](const Vec3& bmin, const Vec3& bmax) {
            if (!(min.x < bmax.x && max.x > bmin.x &&
                  min.y < bmax.y && max.y > bmin.y &&
                  min.z < bmax.z && max.z > bmin.z))
                return;
            // выталкиваем по минимальной оси
            float oxp = bmax.x - min.x, oxm = max.x - bmin.x;
            float oyp = bmax.y - min.y, oym = max.y - bmin.y;
            float ozp = bmax.z - min.z, ozm = max.z - bmin.z;
            float ox = (oxp < oxm) ? -oxp : oxm;
            float oy = (oyp < oym) ? -oyp : oym;
            float oz = (ozp < ozm) ? -ozp : ozm;
            float m = std::fabs(ox), my = std::fabs(oy), mz = std::fabs(oz);
            if (my < m) { m = my; ox = 0; oy = (std::fabs(oyp) < std::fabs(oym)) ? -oyp : oym; }
            if (mz < m) { oz = (std::fabs(ozp) < std::fabs(ozm)) ? -ozp : ozm; ox = 0; oy = 0; }
            pos.x += ox; pos.y += oy; pos.z += oz;
            hitThisIter = true;
            clipped = true;
        };
        for (const auto& b : brushes) testBox(b.min, b.max);
        for (const auto& d : doors) {
            if (d.curOffset >= d.travel - 0.01f) continue; // полностью открыта
            Vec3 mn = d.closedMin, mx = d.closedMax;
            mn.z += d.curOffset; mx.z += d.curOffset;
            testBox(mn, mx);
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

static void doorBox(const Door& d, Vec3& mn, Vec3& mx) {
    mn = d.closedMin; mx = d.closedMax;
    mn.z += d.curOffset; mx.z += d.curOffset;
}

float GameMap::rayCast(const Vec3& o, const Vec3& d, float maxT, Vec3* normal) const {
    float best = -1;
    Vec3 bn;
    auto testBox = [&](const Vec3& bmin, const Vec3& bmax) {
        float t;
        Vec3 n;
        if (rayBox(o, d, bmin, bmax, maxT, t, n)) {
            if (best < 0 || t < best) { best = t; bn = n; }
        }
    };
    for (const auto& b : brushes) testBox(b.min, b.max);
    for (const auto& dr : doors) {
        if (dr.curOffset >= dr.travel - 0.01f) continue;
        Vec3 mn, mx; doorBox(dr, mn, mx);
        testBox(mn, mx);
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
    for (const auto& dr : doors) {
        if (dr.curOffset >= dr.travel - 0.01f) continue;
        Vec3 mn, mx; doorBox(dr, mn, mx);
        if (p.x > mn.x && p.x < mx.x && p.y > mn.y && p.y < mx.y && p.z > mn.z && p.z < mx.z)
            return true;
    }
    (void)o; (void)t;
    return false;
}

void GameMap::updateDoors(float dt) {
    for (auto& d : doors) {
        if (d.animT >= 0.f) {
            d.animT += dt / d.speed;
            if (d.animT >= 1.f) { d.animT = -1.f; d.open = true; d.curOffset = d.travel; }
            else d.curOffset = d.travel * d.animT;
        }
    }
}

bool GameMap::tryUseDoor(const Vec3& playerPos, int keysHave[4], const char*& msgOut) {
    msgOut = nullptr;
    Door* best = nullptr;
    float bestDist = 4.5f; // радиус USE как в quake
    for (auto& d : doors) {
        if (d.animT >= 0.f || d.open) continue;
        // расстояние до AABB двери от центра игрока (по горизонтали+вертикали)
        float dx = std::max({d.closedMin.x - playerPos.x, 0.f, playerPos.x - d.closedMax.x});
        float dy = std::max({d.closedMin.y - playerPos.y, 0.f, playerPos.y - d.closedMax.y});
        float dz = std::max({d.closedMin.z - playerPos.z, 0.f, playerPos.z - d.closedMax.z});
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (dist < bestDist) { bestDist = dist; best = &d; }
    }
    if (!best) return false;
    if (best->keyId == 0) {
        best->animT = 0.f;
        msgOut = "DOOR OPENED";
        return true;
    }
    if (keysHave[best->keyId] > 0) {
        best->animT = 0.f;
        msgOut = best->keyId == 2 ? "GOLD DOOR OPENED" : "SILVER DOOR OPENED";
        return true;
    }
    msgOut = best->keyId == 2 ? "YOU NEED THE GOLD KEY" : "YOU NEED THE SILVER KEY";
    return true;
}
