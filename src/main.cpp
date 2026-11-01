// main.cpp - точка входа: окно, игровой цикл 60Гц, ввод -> InputState, рендер кадра.
// Механика как в Quake 1: bunny-hop, air-strafing, 8 стволов, key pickups, monsters.
#include "game.h"
#include "renderer.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static const int SCR_W = 960;
static const int SCR_H = 600;

static bool g_prevKey[Platform::K_COUNT] = {false};

static void mapInput(Platform& plat, Game& game) {
    InputState& in = game.input;
    auto& ev = plat.events;

    in.fwd   = plat.key(Platform::K_W) || plat.key(Platform::K_UP);
    in.back  = plat.key(Platform::K_S) || plat.key(Platform::K_DOWN);
    in.strafeLeft  = plat.key(Platform::K_A);
    in.strafeRight = plat.key(Platform::K_D);
    in.left = in.strafeLeft; in.right = in.strafeRight;
    in.jump  = plat.key(Platform::K_SPACE);
    in.attack = ev.lmb;
    in.use   = plat.key(Platform::K_E);
    in.mouseDX = ev.dx;
    in.mouseDY = ev.dy;
    ev.dx = ev.dy = 0;

    // переключение оружия цифрами 1..8 (в quake 0/8 -> top up; у нас 8 = plasma)
    in.weaponSwitch = -1;
    static const int kNumKeys[8] = {Platform::K_1, Platform::K_2, Platform::K_3, Platform::K_4,
                                    Platform::K_5, Platform::K_6, Platform::K_7, Platform::K_8};
    for (int i = 0; i < 8; i++) {
        if (plat.key(kNumKeys[i]) && !g_prevKey[kNumKeys[i]]) in.weaponSwitch = i;
        g_prevKey[kNumKeys[i]] = plat.key(kNumKeys[i]);
    }
    // колесо мыши - cycle weapon
    if (ev.wheel > 0) { in.weaponSwitch = -2; ev.wheel = 0; }
    else if (ev.wheel < 0) { in.weaponSwitch = -3; ev.wheel = 0; }

    // R - рестарт после смерти
    if (plat.key(Platform::K_R) && !g_prevKey[Platform::K_R]) {
        if (game.hud.dead) game.restart(), game.started = true;
    }
    g_prevKey[Platform::K_R] = plat.key(Platform::K_R);

    // [ / ] - смена уровня (changelevel как в Quake)
    auto changeLevel = [&](int dir) {
        int nl = game.hud.level + dir;
        if (nl < 1 || nl > GameMap::levelCount()) return;
        game.loadLevel(nl); // loadLevel сам ставит hud.level = nl
    };
    if (plat.key(Platform::K_BRACKET_L) && !g_prevKey[Platform::K_BRACKET_L]) changeLevel(-1);
    if (plat.key(Platform::K_BRACKET_R) && !g_prevKey[Platform::K_BRACKET_R]) changeLevel(+1);
    g_prevKey[Platform::K_BRACKET_L] = plat.key(Platform::K_BRACKET_L);
    g_prevKey[Platform::K_BRACKET_R] = plat.key(Platform::K_BRACKET_R);

    // Enter / LMB - старт из меню
    if (!game.started) {
        if (plat.key(Platform::K_ENTER) || ev.lmbPressed) game.started = true;
    }
    ev.lmbPressed = ev.rmbPressed = false;
}

int main(int argc, char** argv) {
#ifdef _WIN32
    (void)argc; (void)argv;
#else
    if (argc > 1 && std::strcmp(argv[1], "--benchmark") == 0) {
        // headless-прогон: без окна, просто симуляция (для CI)
        Game g; Renderer r; g.init(); r.init(64, 64); g.started = true;
        float dt = 1.f / 60.f;
        for (int i = 0; i < 600; i++) {
            g.input.fwd = true; g.input.jump = (i % 20 == 0);
            g.input.attack = true; g.input.weaponSwitch = W_ROCKET;
            g.hud.weaponsOwned[W_ROCKET] = 1; g.hud.ammo[3] = 99;
            g.update(dt);
            r.render(g);
        }
        printf("BENCHMARK OK time=%.1fs pos=(%.1f,%.1f,%.1f) kills=%d\n",
               g.time, g.player.pos.x, g.player.pos.y, g.player.pos.z, g.hud.kills);
        return 0;
    }
#endif
    Platform plat;
    if (!plat.initWindow(SCR_W, SCR_H, "QUAKELIKE - E1M1 DEAD SIMPLE")) {
        fprintf(stderr, "Failed to create window.\n");
        return 1;
    }

    Game game;
    Renderer renderer;
    game.init();
    renderer.init(SCR_W, SCR_H);

    double prev = getTimeSeconds();
    while (plat.pumpEvents()) {
        double now = getTimeSeconds();
        float dt = (float)(now - prev);
        prev = now;
        if (dt > 0.1f) dt = 0.1f;          // защита от лагов/зависания окна
        if (dt <= 0.f) dt = 1e-4f;

        mapInput(plat, game);
        game.update(dt);
        renderer.render(game);
        plat.present(renderer.framebuffer.data(), SCR_W, SCR_H);

        if (plat.events.quit) break;
        sleepMs(6);                          // ~ vsync-friendly idle
    }

    plat.shutdown();
    return 0;
}
