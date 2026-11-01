// map.h -Brush-based карта (как в Quake: стены-боксы, пол, потолок)
#pragma once
#include "math3d.h"
#include <vector>
#include <cstdint>

struct Brush {
    Vec3 min, max;
    uint32_t color; // RGBA
};

class GameMap {
public:
    std::vector<Brush> brushes;
    float floorZ = 0;
    float ceilZ = 12;

    void buildDemo();

    // Проверка пересечения AABB (игрок/враг) сBrushами. Возвращает выталкивание.
    bool clipHull(Vec3& pos, float radius, float height) const;
    // Ray cast против brush-ов: возвращает t (или -1) и нормаль
    float rayCast(const Vec3& o, const Vec3& d, float maxT, Vec3* normal = nullptr) const;
    // Пересечение AABB с лучом (для снарядов)
    bool rayHitsBrush(const Vec3& o, const Vec3& d, float t, float dt) const;

private:
    bool boxOverlapsAny(const Vec3& min, const Vec3& max) const;
};
