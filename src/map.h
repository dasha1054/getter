// map.h -Brush-based карта (как в Quake: стены-боксы, пол, потолок)
#pragma once
#include "math3d.h"
#include <vector>
#include <cstdint>
#include <functional>

struct Brush {
    Vec3 min, max;
    uint32_t color; // RGBA
};

// Дверь в стиле Quake: блок brush-ов, открывающийся по USE (нужен ключ) или навсегда после открытия
struct Door {
    Vec3 closedMin, closedMax;
    uint32_t color = 0xff8a5038;
    int keyId = 0;          // 0 = без ключа (открывается просто нажатием E), N>0 = нужен ключ N
    bool open = false;      // открыта навсегда (как quake door after trigger)
    float curOffset = 0;    // текущее смещение вверх (в юнитах)
    float travel = 0;       // на сколько поднимать
    float speed = 3.0f;     // сек на полный ход
    float animT = -1.f;     // -1 = не анимируется; иначе прогресс 0..1
    bool forLevel1 = true;
};

class GameMap {
public:
    std::vector<Brush> brushes;
    std::vector<Door> doors;
    float floorZ = 0;
    float ceilZ = 12;
    const char* name = "E1M1 - DEAD SIMPLE";

    void buildLevel(int num); // 1 или 2
    static int levelCount() { return 2; }

private:
    void buildDemo();     // E1M1
    void buildDungeon();  // E1M2
public:

    // Проверка пересечения AABB (игрок/враг) сBrushами. Возвращает выталкивание.
    bool clipHull(Vec3& pos, float radius, float height) const;

    // Активные collision-боксы: brushes + закрытые двери (curOffset < travel)
    void forEachSolid(const std::function<void(const Vec3&, const Vec3&)>& fn) const;
    // Попытка открыть ближайшую дверь по USE; true если что-то произошло (для сообщения)
    bool tryUseDoor(const Vec3& playerPos, int keysHave[4], const char*& msgOut);
    // Обновить анимацию дверей
    void updateDoors(float dt);
    // Ray cast против brush-ов: возвращает t (или -1) и нормаль
    float rayCast(const Vec3& o, const Vec3& d, float maxT, Vec3* normal = nullptr) const;
    // Пересечение AABB с лучом (для снарядов)
    bool rayHitsBrush(const Vec3& o, const Vec3& d, float t, float dt) const;

private:
    bool boxOverlapsAny(const Vec3& min, const Vec3& max) const;
};
