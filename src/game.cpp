// game.cpp - логика: физика Quake (friction/accelerate/air-control/bhop), стрельба, ИИ
#include "game.h"
#include <algorithm>
#include <cstdlib>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
void playSfx(SfxId id) {
    // частоты/длительности в духе Q1 через speaker Beep
    switch (id) {
        case SFX_FIRE:      Beep(700, 30); break;
        case SFX_SHOTGUN:   Beep(250, 60); break;
        case SFX_ROCKET:    Beep(180, 90); break;
        case SFX_PLASMA:    Beep(1200, 40); break;
        case SFX_HIT:       Beep(500, 25); break;
        case SFX_EXPLOSION: Beep(90, 160); break;
        case SFX_PICKUP:    Beep(900, 50); break;
        case SFX_DEATH:     Beep(120, 300); break;
        case SFX_JUMP:      Beep(400, 20); break;
        default: break;
    }
}
#else
#include <cstdio>
void playSfx(SfxId id) { /* без звукового сервера в headless/Linux сборке — no-op */ (void)id; }
#endif

static float frand() { return (float)std::rand() / (float)RAND_MAX; }
static float spreadRand(float s) { return (frand() * 2.f - 1.f) * s; }

void Game::init() { hud = HudState{}; hud.level = 1; loadLevel(1); }

void Game::setMessageTime() { messageTime = 3.f; }

void Game::restart() {
    int lvl = hud.level;
    loadLevel(lvl);
}

void Game::loadLevel(int num) {
    if (num < 1) num = 1;
    if (num > GameMap::levelCount()) num = GameMap::levelCount();
    int keepLevel = num;
    int keys[4] = {0};
    // при changelevel часть статуса сохраняем как в Quake DM-серверных map rotation —
    // здесь: оружие/патроны сбрасываем на стартовые для уровня
    hud = HudState{};
    hud.level = keepLevel;
    (void)keys;
    std::srand(12345 + keepLevel * 777);
    map.buildLevel(keepLevel);
    player = PlayerState{};
    hud.weaponsOwned[W_MELEE] = 1;
    enemies.clear(); pickups.clear(); projectiles.clear(); tracers.clear(); explosions.clear();
    weaponNextFire = 0;
    time = 0;
    message = map.name;
    setMessageTime();

    auto addEnemy = [&](EnemyType t, float x, float y, float z) {
        Enemy e;
        e.type = t;
        e.pos = {x, y, z};
        if (t == EN_GRUNT)   { e.hp = 60;  e.color = 0xff8a9a4a; e.radius = 0.55f; e.height = 1.7f; }
        if (t == EN_ENFORCER){ e.hp = 80;  e.color = 0xffb04a4a; e.radius = 0.55f; e.height = 1.8f; }
        if (t == EN_OGRE)    { e.hp = 200; e.color = 0xff60a060; e.radius = 0.7f;  e.height = 2.0f; }
        enemies.push_back(e);
    };
    auto addPickup = [&](int type, float x, float y, float z, int weapon, int amount) {
        Pickup p; p.type = type; p.pos = {x, y, z}; p.weapon = weapon; p.amount = amount;
        p.bobPhase = frand() * 6.28f;
        pickups.push_back(p);
    };

    if (keepLevel == 1) {
        player.pos = {0, -17, 0.9f};
        player.yaw = 1.5708f;
        addEnemy(EN_GRUNT, -8, 2, 0.9f);
        addEnemy(EN_GRUNT, 8, -4, 0.9f);
        addEnemy(EN_GRUNT, 2, 10, 0.9f);
        addEnemy(EN_ENFORCER, -14, -4, 0.9f);
        addEnemy(EN_ENFORCER, 14, 4, 0.9f);
        addEnemy(EN_ENFORCER, 0, 14, 4.9f);
        addEnemy(EN_OGRE, -6, -12, 0.9f);
        addEnemy(EN_OGRE, 12, 12, 0.9f);

        // Оружие разбросано по карте как в Quake
        addPickup(PICK_WEAPON, -15, 0, 1.0f, W_SHOTGUN, 0);      // комната слева
        addPickup(PICK_WEAPON, 15, 0, 1.0f, W_ROCKET, 0);        // комната справа
        addPickup(PICK_WEAPON, 0, 15, 5.4f, W_PLASMA, 0);         // балкон
        addPickup(PICK_WEAPON, 0, -12, 1.0f, W_MG, 0);            // юг
        addPickup(PICK_WEAPON, -5, 8, 1.0f, W_WS, 0);             // гвоздомёт
        addPickup(PICK_WEAPON, 5, -8, 1.0f, W_SSG, 0);            // двустволка
        addPickup(PICK_WEAPON, -15, 5, 1.0f, W_SUPER_NAIL, 0);    // супер-гвозди
        // Боеприпасы
        addPickup(PICK_SHELLS, -16, -5, 0.9f, 0, 16);
        addPickup(PICK_SHELLS, 16, 5, 0.9f, 0, 16);
        addPickup(PICK_AMMO_BOX, 0, -15, 0.9f, 0, 19);
        addPickup(PICK_AMMO_BOX, 3, -11, 1.8f, 0, 19);
        addPickup(PICK_ROCKETS, 16, -5, 0.9f, 0, 8);
        addPickup(PICK_ROCKETS, 12, 0, 0.9f, 0, 8);
        addPickup(PICK_NAILS, -6, 6, 0.9f, 0, 40);
        addPickup(PICK_NAILS, -15, -2, 0.9f, 0, 40);
        addPickup(PICK_PLASMA, 0, 16, 5.4f, 0, 50);
        addPickup(PICK_PLASMA, -3, 13, 0.9f, 0, 50);
        // Здоровье/броня
        addPickup(PICK_HEALTH_SMALL, 6, 6, 0.9f, 0, 15);
        addPickup(PICK_HEALTH_SMALL, -6, -6, 0.9f, 0, 15);
        addPickup(PICK_HEALTH_LARGE, 0, 0, 2.0f, 0, 25);
        addPickup(PICK_HEALTH_LARGE, -16, 3, 0.9f, 0, 25);
        addPickup(PICK_ARMOR_SMALL, -17, -6, 0.9f, 0, 50);
        addPickup(PICK_ARMOR_COMBAT, -17, 6, 0.9f, 0, 100);
        addPickup(PICK_ARMOR_LARGE, 17, -6, 0.9f, 0, 100);
        addPickup(PICK_QUAD, 0, 17, 5.4f, 0, 0);                  // Quad на балконе
        addPickup(PICK_INVULN, 17, 6, 0.9f, 0, 0);                // Инвул в правой комнате
    } else {
        // E1M2 THE DUNGEON — старт: дробовик, дальше silver key -> gold key
        player.pos = {0, -15, 0.9f};
        player.yaw = 1.5708f;
        hud.weaponsOwned[W_SHOTGUN] = 1;
        hud.currentWeapon = W_SHOTGUN;
        hud.ammo[1] = 20;

        addEnemy(EN_GRUNT, -18, -13, 0.9f);   // левая камера
        addEnemy(EN_GRUNT, 18, -13, 0.9f);    // правая камера
        addEnemy(EN_GRUNT, -8, -2, 0.9f);
        addEnemy(EN_ENFORCER, 8, -2, 0.9f);
        addEnemy(EN_ENFORCER, -12, 2, 0.9f);
        addEnemy(EN_ENFORCER, 12, 14, 0.9f);  // тронный зал
        addEnemy(EN_OGRE, 0, 0, 0.9f);        // центр среднего зала
        addEnemy(EN_OGRE, -6, 15, 2.5f);      // ниша с серебряным ключом
        addEnemy(EN_OGRE, 0, 15.5f, 2.5f);    // у трона

        addPickup(PICK_KEY1, 0, -13, 0.9f, 0, 1);     // silver key в стартовой комнате (открывает 1-ю дверь)
        addPickup(PICK_KEY2, 0, 16.5f, 3.4f, 0, 2);   // gold key в северной нише за 1-й дверью
        addPickup(PICK_WEAPON, 16, -13, 0.9f, W_MG, 0); // пулемёт в правой камере
        addPickup(PICK_WEAPON, -10, -13, 0.9f, W_WS, 0);
        addPickup(PICK_WEAPON, 0, 3, 1.0f, W_ROCKET, 0);       // центр зала
        addPickup(PICK_WEAPON, 0, 15, 3.4f, W_PLASMA, 0);      // ниша
        addPickup(PICK_WEAPON, 0, 16, 2.9f, W_SSG, 0);
        addPickup(PICK_SHELLS, -4, -14, 0.9f, 0, 16);
        addPickup(PICK_SHELLS, 4, -14, 0.9f, 0, 16);
        addPickup(PICK_AMMO_BOX, -12, -13, 0.9f, 0, 19);
        addPickup(PICK_AMMO_BOX, 12, -13, 0.9f, 0, 19);
        addPickup(PICK_ROCKETS, -16, 0, 0.9f, 0, 8);
        addPickup(PICK_ROCKETS, 16, 0, 0.9f, 0, 8);
        addPickup(PICK_NAILS, -8, 3, 0.9f, 0, 40);
        addPickup(PICK_PLASMA, 3, 15, 3.4f, 0, 50);
        addPickup(PICK_HEALTH_SMALL, -17, -13, 0.9f, 0, 15);
        addPickup(PICK_HEALTH_SMALL, 17, -13, 0.9f, 0, 15);
        addPickup(PICK_HEALTH_LARGE, 0, -12, 0.9f, 0, 25);
        addPickup(PICK_HEALTH_LARGE, -4, 15, 3.4f, 0, 25);
        addPickup(PICK_ARMOR_SMALL, 10, -13, 0.9f, 0, 50);
        addPickup(PICK_ARMOR_COMBAT, 0, 13, 2.3f, 0, 100);      // перед троном
        addPickup(PICK_QUAD, 0, 16, 2.9f, 0, 0);                 // на троне!
        addPickup(PICK_INVULN, 18, 15, 0.9f, 0, 0);              // восток тронного зала
    }
}

void Game::tryUse() {
    const char* msg = nullptr;
    if (map.tryUseDoor(player.pos, hud.keys, msg)) {
        if (msg) { message = msg; setMessageTime(); }
        playSfx(msg && msg[0] == 'Y' ? SFX_HIT : SFX_PICKUP); // "YOU NEED..." = отказ
    }
}



// ================= Физика в стиле Quake =================
void Game::applyFriction(float dt) {
    Vec3& v = player.vel;
    float speed = std::sqrt(v.x * v.x + v.y * v.y);
    if (speed < 0.01f) { v.x = 0; v.y = 0; return; }
    float control = std::max(speed, 1.0f);
    float drop = control * FRICTION * dt;
    float newspeed = std::max(0.f, speed - drop) / speed;
    v.x *= newspeed; v.y *= newspeed;
}

void Game::accelerate(const Vec3& wishDir, float wishSpeed, float accel, float dt) {
    // Quake's accelerate(): project current velocity onto wishdir, add up to accel*dt
    float current = player.vel.dot(wishDir);
    float add = wishSpeed - current;
    if (add <= 0) return;
    float accelSpeed = accel * dt * wishSpeed;
    if (add > accelSpeed) add = accelSpeed;
    player.vel += wishDir * add;
}

void Game::tryJump() {
    if (input.jump && player.onGround && !hud.dead) {
        player.vel.z = JUMP_VELOCITY;
        player.onGround = false;
        playSfx(SFX_JUMP);
    }
}

void Game::movePlayer(float dt) {
    // Мышь
    player.yaw += input.mouseDX * 0.0028f;
    player.pitch -= input.mouseDY * 0.0028f;
    const float PITCH_LIMIT = 1.5f;
    player.pitch = std::max(-PITCH_LIMIT, std::min(PITCH_LIMIT, player.pitch));
    input.mouseDX = input.mouseDY = 0;

    // Wish direction на основе yaw
    Vec3 fwd = {std::cos(player.yaw), std::sin(player.yaw), 0};
    Vec3 right = {std::cos(player.yaw - 1.5707963f), std::sin(player.yaw - 1.5707963f), 0};
    Vec3 wishDir{0, 0, 0};
    if (input.fwd)  wishDir += fwd;
    if (input.back) wishDir -= fwd;
    if (input.strafeRight) wishDir += right;
    if (input.strafeLeft)  wishDir -= right;
    // NOTE: left/right (A/D) = strafe; это даёт правильный air-strafing для bhop

    float wishSpeed = wishDir.length();
    if (wishSpeed > 0) wishDir = wishDir / wishSpeed;

    // Прыжок
    tryJump();

    // Гравитация
    player.vel.z -= GRAVITY * dt;

    if (player.onGround) {
        applyFriction(dt);
        if (wishSpeed > 0) {
            float maxs = MAX_GROUND_SPEED;
            if (input.back) maxs *= 0.3f; // медленнее назад? в quake одинаково, оставим полный
            accelerate(wishDir, maxs, ACCEL_GROUND, dt);
        }
    } else {
        // Air control (Quake: PM_AirAccelerate, max vel 30 units/s projection)
        if (wishSpeed > 0) {
            float wishspd = std::min(wishSpeed, 30.f);
            float current = player.vel.dot(wishDir);
            float add = wishspd - current;
            if (add > 0) {
                float accelSpeed = ACCEL_AIR * dt * wishspd;
                if (add > accelSpeed) add = accelSpeed;
                player.vel += wishDir * add;
            }
        }
    }

    // Интеграция с подшагами (step-up до 0.55 юнита — как в quake stepheight 18~0.56u)
    Vec3 oldPos = player.pos;
    Vec3 move = player.vel * dt;
    // Раздельно: сначала XY, потом Z (проще и стабильнее)
    Vec3 test = oldPos;
    test.x += move.x; test.y += move.y;
    bool hitX = false;
    // Подшаг: пробуем приподнять на 0.55 если упёрлись
    Vec3 testUp = test; testUp.z += 0.56f;
    if (!map.clipHull(test, PLAYER_RADIUS, PLAYER_HEIGHT)) {
        // чисто
    } else if (!map.clipHull(testUp, PLAYER_RADIUS, PLAYER_HEIGHT) &&
               player.vel.z <= 0.1f) {
        // забираемся на ступеньку только если стоим/спускаемся
        test = testUp;
        // после подъёма опустимся гравитацией естественно
    } else {
        hitX = true;
    }
    if (hitX) {
        // останавливаем горизонтальную скорость по направлению движения частично
        player.vel.x *= 0.5f; player.vel.y *= 0.5f;
    }

    test.z += move.z;
    bool groundedNow = false;
    if (map.clipHull(test, PLAYER_RADIUS, PLAYER_HEIGHT)) {
        if (move.z < 0) {
            groundedNow = true;
            player.vel.z = 0;
        } else if (move.z > 0) {
            player.vel.z = 0; // потолок
        }
    } else {
        // проверка касания земли: чуть ниже
        Vec3 below = test; below.z -= 0.06f;
        if (map.clipHull(below, PLAYER_RADIUS, PLAYER_HEIGHT) && move.z <= 0) {
            groundedNow = true;
        }
    }
    player.pos = test;
    player.onGround = groundedNow;

    // Bob & view kick
    float hs = std::sqrt(player.vel.x * player.vel.x + player.vel.y * player.vel.y);
    player.speedLines = std::min(1.f, hs / 12.f);
    if (player.onGround && hs > 0.5f) {
        player.bobPhase += dt * hs * 1.4f;
        player.stepPhase += dt * hs;
    }
    player.viewKick *= std::pow(0.0001f, dt); // быстрый спад
    player.fov = 90.f + hs * 1.2f; // лёгкий FOD-kick от скорости (как fast-FW effect)

    // Падение за карту
    if (player.pos.z < -5) damagePlayer(1000, {0, 0, 1});
}

// ================= Стрельба =================
int Game::ammoIndexForWeapon(int w) const {
    switch (w) {
        case W_SHOTGUN: case W_SSG: return 1; // shells
        case W_MG: return 0;                  // bullets (в quake MG использует cells? нет - bullets из ammo box)
        case W_WS: case W_SUPER_NAIL: return 2; // nails
        case W_ROCKET: return 3;              // rockets
        case W_PLASMA: return 4;              // plasma cells
        default: return 0;
    }
}

void Game::giveAmmo(int weaponId, int amount) {
    int idx = ammoIndexForWeapon(weaponId);
    hud.ammo[idx] = std::min(hud.ammo[idx] + amount, 250);
}

void Game::fireWeapon() {
    if (hud.dead) return;
    int w = hud.currentWeapon;
    const WeaponDef& wd = kWeapons[w];
    if (time < weaponNextFire) return;
    int ai = ammoIndexForWeapon(w);
    if (w != W_MELEE && hud.ammo[ai] < wd.ammoPerShot) {
        // автопереключение на кулаки как в Quake
        hud.currentWeapon = W_MELEE;
        return;
    }
    if (w != W_MELEE) hud.ammo[ai] -= wd.ammoPerShot;
    weaponNextFire = time + wd.fireInterval;
    player.viewKick = 1.f;
    switch (w) {
        case W_SHOTGUN: case W_SSG: playSfx(SFX_SHOTGUN); break;
        case W_ROCKET:              playSfx(SFX_ROCKET);  break;
        case W_PLASMA:              playSfx(SFX_PLASMA);  break;
        default:                    playSfx(SFX_FIRE);    break;
    }

    Vec3 eye = player.pos + Vec3{0, 0, PLAYER_HEIGHT * 0.5f - 0.1f};
    Vec3 baseDir = dirFromAngles(player.yaw, player.pitch);
    float dmgMul = hud.quadTime > 0 ? 4.f : 1.f;

    if (wd.projectileSpeed <= 0.f) {
        // hitscan (или melee)
        for (int i = 0; i < wd.pellets; i++) {
            Vec3 dir = baseDir;
            if (wd.spread > 0) {
                Vec3 r1 = Vec3{-baseDir.y, baseDir.x, 0}.normalized();
                Vec3 r2 = dir.cross(r1).normalized();
                float a = frand() * 6.2831853f;
                float rr = std::sqrt(frand()) * wd.spread;
                dir = (dir + r1 * (rr * std::cos(a)) + r2 * (rr * std::sin(a))).normalized();
            }
            hitscanShot(eye, dir, wd.damagePerHit * dmgMul, wd.color);
        }
    } else {
        Vec3 dir = baseDir;
        if (wd.spread > 0) {
            Vec3 r1 = Vec3{-baseDir.y, baseDir.x, 0}.normalized();
            Vec3 r2 = dir.cross(r1).normalized();
            float a = frand() * 6.2831853f;
            float rr = std::sqrt(frand()) * wd.spread;
            dir = (dir + r1 * (rr * std::cos(a)) + r2 * (rr * std::sin(a))).normalized();
        }
        spawnProjectile(eye + dir * 0.4f, dir, wd, dmgMul);
    }
}

void Game::spawnProjectile(const Vec3& origin, const Vec3& dir, const WeaponDef& wd, float dmgMul) {
    Projectile p;
    p.pos = origin;
    p.vel = dir * wd.projectileSpeed;
    p.damage = wd.damagePerHit * dmgMul;
    p.life = 4.f;
    p.fromPlayer = true;
    p.splashRadius = wd.splashRadius;
    p.color = wd.color;
    p.radius = (wd.splashRadius > 0) ? 0.25f : 0.12f;
    projectiles.push_back(p);
}

Enemy* Game::traceEnemyHit(const Vec3& o, const Vec3& d, float maxT, float& tOut) {
    Enemy* best = nullptr;
    tOut = maxT;
    for (auto& e : enemies) {
        if (!e.alive) continue;
        // sphere/capsule тест: closest point на луче к центру врага
        Vec3 c = e.pos + Vec3{0, 0, e.height * 0.4f};
        Vec3 oc = c - o;
        float t = oc.dot(d);
        if (t < 0 || t > tOut) continue;
        Vec3 cp = o + d * t;
        float dx = cp.x - e.pos.x, dy = cp.y - e.pos.y;
        float horizDist2 = dx * dx + dy * dy;
        float zlo = e.pos.z - e.height * 0.5f, zhi = e.pos.z + e.height * 0.5f;
        float cz = cp.z;
        float dz = (cz < zlo) ? (zlo - cz) : (cz > zhi ? cz - zhi : 0);
        if (horizDist2 + dz * dz <= e.radius * e.radius) {
            best = &e; tOut = t;
        }
    }
    return best;
}

void Game::hitscanShot(const Vec3& origin, const Vec3& dir, float damage, uint32_t color) {
    float wallT = map.rayCast(origin, dir, 60.f);
    float enemyT = 1e9f;
    Enemy* hit = traceEnemyHit(origin, dir, wallT > 0 ? wallT : 60.f, enemyT);
    Vec3 end = origin + dir * ((hit && wallT < 0) ? enemyT : (wallT > 0 ? wallT : 60.f));
    if (hit) {
        end = origin + dir * enemyT;
        hit->hp -= damage;
        hit->painFlash = 0.15f;
        // knockback
        hit->vel += dir * (damage * 0.02f);
        if (hit->hp <= 0) { hit->alive = false; hit->deathTime = time; hud.kills++; playSfx(SFX_DEATH); }
        else playSfx(SFX_HIT);
    }
    tracers.push_back({origin, end, 0.06f, color});
}

void Game::explodeAt(const Vec3& pos, float radius, float damage, bool fromPlayer) {
    explosions.push_back({pos, 0.f, radius});
    playSfx(SFX_EXPLOSION);
    // урон врагам
    for (auto& e : enemies) {
        if (!e.alive) continue;
        float d = (e.pos + Vec3{0, 0, e.height * 0.3f} - pos).length();
        if (d < radius) {
            float f = 1.f - d / radius;
            e.hp -= damage * f;
            Vec3 kb = (e.pos - pos).normalized();
            e.vel += kb * (f * 8.f) + Vec3{0, 0, f * 4.f};
            if (e.hp <= 0) { e.alive = false; e.deathTime = time; if (fromPlayer) hud.kills++; }
        }
    }
    // self-damage rocket jump! (только если игрок рядом и снаряд его)
    float pd = (player.pos - pos).length();
    if (pd < radius) {
        float f = 1.f - pd / radius;
        Vec3 push = (player.pos - pos).normalized();
        if (push.lengthSquared() < 0.1f) push = {0, 0, 1};
        player.vel += push * (f * 14.f);
        player.vel.z = std::max(player.vel.z, f * 10.f);
        damagePlayer(damage * f * 0.5f, -push);
    }
}

void Game::damagePlayer(float amount, const Vec3& fromDir) {
    if (hud.invulnTime > 0 || hud.dead) return;
    // armor absorb: yellow 30%, combat 50% (как в quake)
    if (hud.armor > 0) {
        float absorb = (hud.armorType >= 2) ? 0.5f : 0.3f;
        float absorbed = std::min(hud.armor, amount * absorb);
        hud.armor -= absorbed;
        amount -= absorbed;
    }
    hud.health -= amount;
    hud.damageFlash = std::min(1.f, hud.damageFlash + amount / 40.f);
    // slight knock view
    (void)fromDir;
    if (hud.health <= 0) {
        hud.health = 0;
        hud.dead = true;
        message = "YOU DIED - press R to restart"; setMessageTime();
    }
}

// ================= Обновление мира =================
void Game::updateEnemies(float dt) {
    Vec3 eye = player.pos + Vec3{0, 0, PLAYER_HEIGHT * 0.5f};
    for (auto& e : enemies) {
        if (!e.alive) continue;
        Vec3 toPlayer = eye - e.pos;
        float dist = toPlayer.length();
        Vec3 dir = dist > 0.01f ? toPlayer / dist : Vec3{1, 0, 0};
        e.yaw = std::atan2(dir.y, dir.x);

        // ИИ: движение к игроку, атака
        float desiredSpeed = 0;
        if (dist < 25.f) {
            // LOS check
            float wallT = map.rayCast(e.pos + Vec3{0, 0, e.height * 0.4f}, dir, dist);
            bool los = (wallT < 0 || wallT > dist - 0.5f);
            if (e.type == EN_GRUNT) {
                if (dist > 1.4f) desiredSpeed = 3.2f;
                if (los && dist < 1.8f) {
                    e.fireCooldown -= dt;
                    if (e.fireCooldown <= 0) {
                        e.fireCooldown = 1.2f;
                        damagePlayer(10.f, -dir);
                    }
                }
            } else if (e.type == EN_ENFORCER) {
                if (dist > 8.f) desiredSpeed = 2.6f;
                else if (dist < 5.f) desiredSpeed = -2.0f;
                if (los) {
                    e.fireCooldown -= dt;
                    if (e.fireCooldown <= 0) {
                        e.fireCooldown = 1.4f;
                        WeaponDef fake; fake.projectileSpeed = 18.f; fake.damagePerHit = 8.f;
                        fake.splashRadius = 0; fake.color = 0xffff6060;
                        Vec3 md = (dir + Vec3{spreadRand(0.04f), spreadRand(0.04f), spreadRand(0.04f)}).normalized();
                        Projectile p;
                        p.pos = e.pos + Vec3{0, 0, e.height * 0.4f} + md * 0.5f;
                        p.vel = md * fake.projectileSpeed;
                        p.damage = fake.damagePerHit;
                        p.life = 4.f; p.fromPlayer = false; p.color = fake.color; p.radius = 0.15f;
                        projectiles.push_back(p);
                    }
                }
            } else { // OGRE: медленный, тяжёлые fireballs со splash
                if (dist > 6.f) desiredSpeed = 2.0f;
                if (los) {
                    e.fireCooldown -= dt;
                    if (e.fireCooldown <= 0) {
                        e.fireCooldown = 2.2f;
                        Vec3 md = (dir + Vec3{spreadRand(0.03f), spreadRand(0.03f), spreadRand(0.05f)}).normalized();
                        Projectile p;
                        p.pos = e.pos + Vec3{0, 0, e.height * 0.5f} + md * 0.6f;
                        p.vel = md * 14.f;
                        p.damage = 40.f;
                        p.life = 4.f; p.fromPlayer = false; p.color = 0xffff9030;
                        p.splashRadius = 2.4f; p.radius = 0.25f;
                        projectiles.push_back(p);
                    }
                }
            }
        }
        // движение
        Vec3 moveDir = dir; moveDir.z = 0;
        if (desiredSpeed < 0) moveDir = -moveDir;
        e.vel.x += moveDir.x * desiredSpeed * 4.f * dt;
        e.vel.y += moveDir.y * desiredSpeed * 4.f * dt;
        // торможение
        e.vel.x *= std::pow(0.02f, dt);
        e.vel.y *= std::pow(0.02f, dt);
        e.vel.z -= GRAVITY * dt;

        Vec3 np = e.pos + e.vel * dt;
        map.clipHull(np, e.radius, e.height);
        if (np.z <= 0.9f && e.vel.z < 0) { np.z = std::max(np.z, 0.9f); e.vel.z = 0; }
        e.pos = np;
        e.painFlash = std::max(0.f, e.painFlash - dt);
    }
}

void Game::updateProjectiles(float dt) {
    for (size_t i = 0; i < projectiles.size(); i++) {
        Projectile& p = projectiles[i];
        p.life -= dt;
        Vec3 step = p.vel * dt;
        float stepLen = step.length();
        bool removed = false;
        // sub-step movement
        int sub = std::max(1, (int)(stepLen / 0.2f));
        for (int s = 0; s < sub && !removed; s++) {
            Vec3 prev = p.pos;
            p.pos += step / sub;
            // стены
            if (map.rayHitsBrush(prev, p.vel, 0, (s + 1) * dt)) {
                if (p.splashRadius > 0) explodeAt(p.pos, p.splashRadius, p.damage, p.fromPlayer);
                projectiles.erase(projectiles.begin() + i); removed = true; break;
            }
            if (p.fromPlayer) {
                for (auto& e : enemies) {
                    if (!e.alive) continue;
                    Vec3 c = e.pos + Vec3{0, 0, e.height * 0.4f} - p.pos;
                    float dx = c.x, dy = c.y;
                    float hd2 = dx * dx + dy * dy;
                    float dz = std::max(std::abs(c.z) - e.height * 0.5f, 0.f);
                    float rr = e.radius + p.radius;
                    if (hd2 + dz * dz < rr * rr) {
                        if (p.splashRadius > 0) explodeAt(p.pos, p.splashRadius, p.damage, true);
                        else {
                            e.hp -= p.damage;
                            e.painFlash = 0.15f;
                            e.vel += p.vel.normalized() * (p.damage * 0.02f);
                            if (e.hp <= 0) { e.alive = false; e.deathTime = time; hud.kills++; }
                        }
                        projectiles.erase(projectiles.begin() + i); removed = true; break;
                    }
                }
            } else {
                // вражеский снаряд против игрока
                Vec3 c = player.pos - p.pos;
                float hd2 = c.x * c.x + c.y * c.y;
                float dz = std::max(std::abs(c.z) - PLAYER_HEIGHT * 0.5f, 0.f);
                float rr = PLAYER_RADIUS + p.radius;
                if (hd2 + dz * dz < rr * rr) {
                    if (p.splashRadius > 0) explodeAt(p.pos, p.splashRadius, p.damage, false);
                    else damagePlayer(p.damage, -p.vel.normalized());
                    projectiles.erase(projectiles.begin() + i); removed = true; break;
                }
            }
        }
        if (!removed && p.life <= 0) {
            if (p.splashRadius > 0) explodeAt(p.pos, p.splashRadius, p.damage, p.fromPlayer);
            projectiles.erase(projectiles.begin() + i);
        }
    }
}

void Game::updatePickups(float dt) {
    for (auto& pk : pickups) {
        if (pk.taken) {
            if (pk.respawnTime > 0) {
                pk.respawnTime -= dt;
                if (pk.respawnTime <= 0) pk.taken = false;
            }
            continue;
        }
        float dx = pk.pos.x - player.pos.x, dy = pk.pos.y - player.pos.y;
        float dz = pk.pos.z - player.pos.z;
        if (dx * dx + dy * dy < 1.1f && std::abs(dz) < 1.6f && !hud.dead) {
            bool got = true;
            switch (pk.type) {
                case PICK_HEALTH_SMALL:
                    if (hud.health >= 25) got = false; else { hud.health = std::min(25.f, hud.health + 15); message = "+15 health"; }
                    break;
                case PICK_HEALTH_LARGE:
                    if (hud.health >= 100) got = false; else { hud.health = std::min(100.f, hud.health + 25); message = "+25 health"; }
                    break;
                case PICK_ARMOR_SMALL:
                    if (hud.armor >= 50 && hud.armorType >= 1) got = false;
                    else { hud.armor = std::max(hud.armor, 50.f); hud.armorType = 1; message = "Armor"; }
                    break;
                case PICK_ARMOR_LARGE:
                    if (hud.armor >= 100 && hud.armorType >= 1) got = false;
                    else { hud.armor = 100.f; hud.armorType = 1; message = "Large Armor"; }
                    break;
                case PICK_ARMOR_COMBAT:
                    if (hud.armor >= 100 && hud.armorType >= 2) got = false;
                    else { hud.armor = std::max(hud.armor, 100.f); hud.armorType = 2; message = "Combat Armor"; }
                    break;
                case PICK_AMMO_BOX: giveAmmo(W_MG, pk.amount); message = "+Bullets"; break;
                case PICK_SHELLS:   giveAmmo(W_SHOTGUN, pk.amount); message = "+Shells"; break;
                case PICK_NAILS:    giveAmmo(W_WS, pk.amount); message = "+Nails"; break;
                case PICK_ROCKETS:  giveAmmo(W_ROCKET, pk.amount); message = "+Rockets"; break;
                case PICK_PLASMA:   giveAmmo(W_PLASMA, pk.amount); message = "+Cells"; break;
                case PICK_QUAD:     hud.quadTime = 45.f; message = "QUAD DAMAGE!"; break;
                case PICK_INVULN:   hud.invulnTime = 30.f; message = "INVULNERABILITY!"; break;
                case PICK_KEY1:     hud.keys[1]++; message = "SILVER KEY"; break;
                case PICK_KEY2:     hud.keys[2]++; message = "GOLD KEY"; break;
                case PICK_WEAPON: {
                    int w = pk.weapon;
                    if (!hud.weaponsOwned[w]) {
                        hud.weaponsOwned[w] = 1;
                        hud.currentWeapon = w;
                        giveAmmo(w, 20);
                        message = std::string("Got the ") + kWeapons[w].name;
                    } else {
                        giveAmmo(w, 10);
                        got = false; // не забираем, пусть лежит
                        if (hud.ammo[ammoIndexForWeapon(w)] >= 250) got = true;
                        else got = false;
                    }
                    break;
                }
            }
            if (got) {
                pk.taken = true;
                pk.respawnTime = (pk.type == PICK_QUAD || pk.type == PICK_INVULN) ? 0.f : 20.f;
                if (pk.type == PICK_QUAD || pk.type == PICK_INVULN) pk.respawnTime = 300.f;
                hud.pickupsCollected++;
                playSfx(SFX_PICKUP);
                setMessageTime();
            }
        }
    }
}

void Game::update(float dt) {
    time += dt;
    if (!started) return;

    if (hud.dead) {
        // смерть: всё ещё летим вниз
        player.vel.z -= GRAVITY * dt;
        player.pos += player.vel * dt;
        return;
    }

    hud.timeAlive += dt;
    hud.quadTime = std::max(0.f, hud.quadTime - dt);
    hud.invulnTime = std::max(0.f, hud.invulnTime - dt);
    hud.damageFlash = std::max(0.f, hud.damageFlash - dt * 2.f);
    messageTime -= dt;

    // переключение оружия
    if (input.weaponSwitch >= 0 && input.weaponSwitch < W_COUNT) {
        if (hud.weaponsOwned[input.weaponSwitch]) hud.currentWeapon = input.weaponSwitch;
        input.weaponSwitch = -1;
    }
    // колесо прокрутки / mwheel
    if (input.weaponSwitch == -2) { // next
        input.weaponSwitch = -1;
        for (int i = 1; i <= W_COUNT; i++) {
            int w = (hud.currentWeapon + i) % W_COUNT;
            if (hud.weaponsOwned[w]) { hud.currentWeapon = w; break; }
        }
    } else if (input.weaponSwitch == -3) { // prev
        input.weaponSwitch = -1;
        for (int i = 1; i <= W_COUNT; i++) {
            int w = (hud.currentWeapon - i + W_COUNT * 2) % W_COUNT;
            if (hud.weaponsOwned[w]) { hud.currentWeapon = w; break; }
        }
    }

    movePlayer(dt);

    // USE (E) — открывание дверей; edge-триггер через prevUse
    static bool prevUse = false;
    if (input.use && !prevUse) tryUse();
    prevUse = input.use;

    // стрельба
    const WeaponDef& wd = kWeapons[hud.currentWeapon];
    if (input.attack && (wd.automatic || time >= weaponNextFire)) fireWeapon();

    map.updateDoors(dt);
    updateEnemies(dt);
    updateProjectiles(dt);
    updatePickups(dt);

    // эффекты затухают
    for (auto& t : tracers) t.life -= dt;
    tracers.erase(std::remove_if(tracers.begin(), tracers.end(),
                                 [](const Tracer& t) { return t.life <= 0; }), tracers.end());
    for (auto& ex : explosions) ex.t += dt;
    explosions.erase(std::remove_if(explosions.begin(), explosions.end(),
                                    [](const Explosion& e) { return e.t > 0.35f; }), explosions.end());
}
