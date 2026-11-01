// math3d.h - минимальная 3D-математика (Quake-style)
#pragma once
#include <cmath>

struct Vec3 {
    float x = 0, y = 0, z = 0;

    Vec3() {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3 cross(const Vec3& o) const {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }
    float length() const { return std::sqrt(dot(*this)); }
    float lengthSquared() const { return dot(*this); }
    Vec3 normalized() const {
        float l = length();
        if (l < 1e-8f) return {0, 0, 0};
        return *this / l;
    }
};

inline Vec3 operator*(float s, const Vec3& v) { return v * s; }
inline Vec3 operator-(const Vec3& v) { return {-v.x, -v.y, -v.z}; }

struct Ray {
    Vec3 origin;
    Vec3 dir; // нормализован
};

// Камера: yaw - поворот вокруг Z (горизонталь), pitch - наклон (вертикаль)
inline Vec3 dirFromAngles(float yaw, float pitch) {
    float cp = std::cos(pitch);
    return {cp * std::cos(yaw), cp * std::sin(yaw), std::sin(pitch)};
}
