// weapons.cpp - таблица оружия в стиле Quake 1
#include "types.h"

const WeaponDef kWeapons[W_COUNT] = {
    // name              short    ammo/shot dmg   pellets speed auto interval spread splash color
    {"Fist",             "FIST",  0,        15.f, 1,      0.f, false, 0.5f,  0.f,   0.f,   0xffffffffu},
    {"Shotgun",          "SG",    1,        4.f,  9,      0.f, false, 0.62f, 0.05f, 0.f,   0xffd8b060u},
    {"Double Shotgun",   "DSG",   2,        4.f,  16,     0.f, false, 0.72f, 0.07f, 0.f,   0xffd8b060u},
    {"Machinegun",       "MG",    1,        8.f,  1,      0.f, true,  0.13f, 0.03f, 0.f,   0xffffe0a0u},
    {"Nailgun",          "NG",    1,        10.f, 1,      35.f,false, 0.22f, 0.01f, 0.f,   0xffc0c0c0u},
    {"Super Nailgun",    "SNG",   1,        10.f, 1,      45.f,true,  0.11f, 0.02f, 0.f,   0xffe0e0e0u},
    {"Rocket Launcher",  "RL",    1,        100.f,1,      22.f,false, 0.6f,  0.f,   3.2f,  0xffff8030u},
    {"Plasma Blaster",   "PB",    1,        20.f, 1,      40.f,true,  0.1f,  0.01f, 0.6f,  0xff60c0ffu},
};
