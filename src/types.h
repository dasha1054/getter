// types.h - общие игровые типы: оружие, предметы, враги, HUD-информация
#pragma once
#include "math3d.h"
#include <cstdint>

// ---- Идентификаторы оружия (Quake 1) ----
enum WeaponId : int {
    W_MELEE = 0,   // Кулак / топор
    W_SHOTGUN,     // Дробовик
    W_SSG,         // Двуствольный дробовик
    W_MG,          // Пулемёт (machinegun)
    W_WS,          // Гвоздомёт (wallgun / nailgun)
    W_SUPER_NAIL,  // Супергвоздомёт
    W_ROCKET,      // Ракетомёт
    W_PLASMA,      // Плазмабластер
    W_COUNT
};

struct WeaponDef {
    const char* name;
    const char* shortName;
    int ammoPerShot;
    float damagePerHit;   // урон за пулю/осколок
    int pellets;          // число снарядов за выстрел (дробовик)
    float projectileSpeed;// 0 = hitscan
    bool automatic;       // зажать = стрелять
    float fireInterval;   // сек между выстрелами
    float spread;         // разброс в радианах
    float splashRadius;   // радиус взрыва (ракеты)
    uint32_t color;       // цвет трассера/снаряда RGBA
};

extern const WeaponDef kWeapons[W_COUNT];

// ---- Предметы ----
enum PickupType : int {
    PICK_HEALTH_SMALL = 0, // +15
    PICK_HEALTH_LARGE,     // +25
    PICK_ARMOR_SMALL,      // +50 armor
    PICK_ARMOR_LARGE,      // +100 armor
    PICK_ARMOR_COMBAT,     // combat armor (max 100, better absorb)
    PICK_AMMO_BOX,         // +пули
    PICK_ROCKETS,          // +ракеты
    PICK_NAILS,            // +гвозди
    PICK_PLASMA,           // +плазма
    PICK_SHELLS,           // +дробь
    PICK_WEAPON,           // оружие (weapon field)
    PICK_INVULN,           // Pentagram of Protection
    PICK_QUAD,             // Quad Damage
    PICK_COUNT
};

struct Pickup {
    Vec3 pos;
    int type = 0;
    int weapon = 0;    // для PICK_WEAPON
    int amount = 0;    // сколько даёт
    bool taken = false;
    float respawnTime = 0.f; // когда возродится (0 = сразу)
    float bobPhase = 0.f;
};

// ---- Враги ----
enum EnemyType : int {
    EN_GRUNT = 0,  // медленный, ближний бой
    EN_ENFORCER,   // стреляет снарядами
    EN_OGRE,       // сильный, мечет fireballs
    EN_COUNT
};

struct Enemy {
    EnemyType type = EN_GRUNT;
    Vec3 pos;
    Vec3 vel;
    float hp = 30;
    float radius = 0.6f;
    float height = 1.7f;
    float yaw = 0;
    float fireCooldown = 0;
    float painFlash = 0;
    bool alive = true;
    float deathTime = 0;
    uint32_t color;
};

// ---- Снаряды игрока/врагов ----
struct Projectile {
    Vec3 pos;
    Vec3 vel;
    float damage = 0;
    float life = 0;
    bool fromPlayer = true;
    float splashRadius = 0;
    uint32_t color = 0xffffffff;
    float radius = 0.15f;
};

// ---- HUD ----
struct HudState {
    float health = 100;
    float armor = 0;
    int armorType = 0;
    int currentWeapon = 0;
    int weaponsOwned[W_COUNT] = {0};
    int ammo[W_COUNT] = {0}; // боезапас по типам: bullets/shells/nails/rockets/plasma
    float quadTime = 0;
    float invulnTime = 0;
    float damageFlash = 0;
    int kills = 0;
    int pickupsCollected = 0;
    bool dead = false;
    float timeAlive = 0;
};
