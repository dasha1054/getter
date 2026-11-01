// game.h - игровое состояние: игрок, физика (bunny-hop, air-control как в Quake), оружие, враги
#pragma once
#include "math3d.h"
#include "types.h"
#include "map.h"
#include <vector>
#include <string>

struct InputState {
    bool fwd = false, back = false, left = false, right = false;
    bool jump = false, attack = false;
    bool use = false;
    float mouseDX = 0, mouseDY = 0;
    int weaponSwitch = -1; // 0..7 или -1
    bool strafeLeft = false, strafeRight = false; // A/D strafing для bunny-hop
};

struct Tracer {
    Vec3 a, b;
    float life;
    uint32_t color;
};

struct Explosion {
    Vec3 pos;
    float t;      // время жизни
    float radius;
};

struct PlayerState {
    Vec3 pos{0, -17, 0.9f};
    Vec3 vel{0, 0, 0};
    float yaw = 1.5708f;   // смотрит на север
    float pitch = 0;
    bool onGround = false;
    float stepPhase = 0;
    float viewKick = 0;
    float bobPhase = 0;
    float fov = 90.f;
    float speedLines = 0; // ощущение скорости
};

class Game {
public:
    GameMap map;
    PlayerState player;
    HudState hud;
    InputState input;

    std::vector<Enemy> enemies;
    std::vector<Pickup> pickups;
    std::vector<Projectile> projectiles;
    std::vector<Tracer> tracers;
    std::vector<Explosion> explosions;

    float time = 0;
    float weaponNextFire = 0;
    std::string message;
    void setMessageTime();
    float messageTime = 0;
    bool started = false;
    bool showScores = false;

    void init();
    void restart();
    void update(float dt);

    static constexpr float PLAYER_RADIUS = 0.45f;
    static constexpr float PLAYER_HEIGHT = 1.8f;
    static constexpr float GRAVITY = 20.0f;      // ~ quake-like при масштабе 1u=~32qu
    static constexpr float JUMP_VELOCITY = 6.2f; // прыжок ~ как в quakе (350 qu/sec)
    static constexpr float MAX_GROUND_SPEED = 7.0f; // 320 units/s
    static constexpr float ACCEL_GROUND = 60.f;     //wishper-ускорение
    static constexpr float ACCEL_AIR = 12.f;        // air control (Quake: 0.1 * accel)
    static constexpr float FRICTION = 6.5f;         // трение на земле
    static constexpr float AIR_FRICTION = 0.f;

private:
    // физика в стиле Quake
    void applyFriction(float dt);
    void accelerate(const Vec3& wishDir, float wishSpeed, float accel, float dt);
    void airAccelerate(const Vec3& wishDir, float wishSpeed, float dt);
    void tryJump();
    void movePlayer(float dt);

    void fireWeapon();
    void spawnProjectile(const Vec3& origin, const Vec3& dir, const WeaponDef& wd, float dmgMul);
    void hitscanShot(const Vec3& origin, const Vec3& dir, float damage, uint32_t color);
    void explodeAt(const Vec3& pos, float radius, float damage, bool fromPlayer);

    void updateEnemies(float dt);
    void updateProjectiles(float dt);
    void updatePickups(float dt);
    void damagePlayer(float amount, const Vec3& fromDir);

    Enemy* traceEnemyHit(const Vec3& o, const Vec3& d, float maxT, float& tOut);

    int ammoIndexForWeapon(int w) const; // тип боезапаса
    void giveAmmo(int weaponId, int amount);
};
