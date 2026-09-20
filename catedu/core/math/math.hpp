///
/// BRIEF: Collection of math types and functions
///

#pragma once
// NOTE: Oh I hate you Microsoft SO much
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif

#include <cmath>
#include <float.h>
#include <stdint.h>

#pragma region constants
constexpr float MATH_TAU = 6.283185307179586;
constexpr float MATH_PI = 3.141592653589793;
constexpr float MATH_HALF_PI = 1.5707963267948966;
constexpr float MATH_DEG_TO_RAD = MATH_PI / 180.0f;
constexpr float MATH_RAD_TO_DEG = 180.0f / MATH_PI;
constexpr float MATH_EPSILON = FLT_MIN;
constexpr float MATH_INFINITY = FLT_MAX;
constexpr float MATH_E = 2.718281828459045;
#pragma endregion

struct Vector2i
{
    union {
        struct
        {
            int32_t x, y;
        };
        int32_t data[2];
    };
};

inline Vector2i operator+(Vector2i a, Vector2i b)
{
    return {a.x + b.x, a.y + b.y};
}

inline Vector2i operator-(Vector2i a, Vector2i b)
{
    return {a.x - b.x, a.y - b.y};
}

inline Vector2i operator*(Vector2i a, Vector2i b)
{
    return {a.x * b.x, a.y * b.y};
}

inline Vector2i operator/(Vector2i a, Vector2i b)
{
    return {a.x / b.x, a.y / b.y};
}

inline Vector2i operator*(Vector2i a, int32_t b)
{
    return {a.x * b, a.y * b};
}

inline Vector2i operator/(Vector2i a, int32_t b)
{
    return {a.x / b, a.y / b};
}

inline Vector2i operator+=(Vector2i &a, Vector2i b)
{
    return a = {a.x + b.x, a.y + b.y};
}

inline Vector2i operator-=(Vector2i &a, Vector2i b)
{
    return a = {a.x - b.x, a.y - b.y};
}

inline Vector2i operator*=(Vector2i &a, Vector2i b)
{
    return a = {a.x * b.x, a.y * b.y};
}

inline Vector2i operator/=(Vector2i &a, Vector2i b)
{
    return a = {a.x / b.x, a.y / b.y};
}

inline Vector2i operator*=(Vector2i &a, int32_t b)
{
    return a = {a.x * b, a.y * b};
}

inline Vector2i operator/=(Vector2i &a, int32_t b)
{
    return a = {a.x / b, a.y / b};
}

inline Vector2i operator%(Vector2i &a, int32_t b)
{
    return {a.x % b, a.y % b};
}

inline Vector2i operator%(Vector2i &a, Vector2i b)
{
    return {a.x % b.x, a.y % b.y};
}

inline Vector2i operator%=(Vector2i &a, int32_t b)
{
    return a = {a.x % b, a.y % b};
}

inline Vector2i operator%=(Vector2i &a, Vector2i b)
{
    return a = {a.x % b.x, a.y % b.y};
}

inline bool operator==(const Vector2i &a, Vector2i b)
{
    return a.x == b.x && a.y == b.y;
}

inline bool operator!=(const Vector2i &a, Vector2i b)
{
    return a.x != b.x || a.y != b.y;
}

struct Vector2
{
    union {
        struct
        {
            float x, y;
        };
        float values[2];
    };
};

inline Vector2 operator-(Vector2 a)
{
    return {-a.x, -a.y};
}

inline Vector2 operator+(Vector2 a, Vector2 b)
{
    return {a.x + b.x, a.y + b.y};
}

inline Vector2 operator+=(Vector2 &a, Vector2 b)
{
    return a = {a.x + b.x, a.y + b.y};
}

inline Vector2 operator-(Vector2 a, Vector2 b)
{
    return {a.x - b.x, a.y - b.y};
}

inline Vector2 operator-=(Vector2 &a, Vector2 b)
{
    return a = {a.x - b.x, a.y - b.y};
}

inline Vector2 operator*(Vector2 a, Vector2 b)
{
    return {a.x * b.x, a.y * b.y};
}

inline Vector2 operator*(Vector2 a, float b)
{
    return {a.x * b, a.y * b};
}

inline Vector2 operator*=(Vector2 &a, Vector2 b)
{
    return a = {a.x * b.x, a.y * b.y};
}

inline Vector2 operator*=(Vector2 &a, float b)
{
    return a = {a.x * b, a.y * b};
}

inline Vector2 operator/(Vector2 a, Vector2 b)
{
    return {a.x / b.x, a.y / b.y};
}

inline Vector2 operator/(Vector2 a, float b)
{
    return {a.x / b, a.y / b};
}

inline Vector2 operator/=(Vector2 &a, float b)
{
    return a = {a.x / b, a.y / b};
}

inline Vector2 operator/=(Vector2 &a, Vector2 b)
{
    return a = {a.x / b.x, a.y / b.y};
}

struct Vector3
{
    union {
        struct
        {
            float x, y, z;
        };
        float values[3];
    };
};

struct Vector4
{
    union {
        struct
        {
            float x, y, z, w;
        };
        float values[4];
    };
};

struct Ray3
{
    Vector3 origin;
    Vector3 direction;
};

struct Box3
{
    Vector3 min;
    Vector3 max;
};

struct Rect
{
    Vector2 pos;
    Vector2 siz;
};

struct RectI
{
    Vector2i pos;
    Vector2i siz;
};

inline Vector3 operator+(Vector3 a, Vector3 b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vector3 operator-(Vector3 a, Vector3 b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Vector3 operator*(Vector3 a, Vector3 b)
{
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}

inline Vector3 operator/(Vector3 a, Vector3 b)
{
    return {a.x / b.x, a.y / b.y, a.z / b.z};
}

inline Vector3 operator*(Vector3 a, float b)
{
    return {a.x * b, a.y * b, a.z * b};
}

inline Vector3 operator/(Vector3 a, float b)
{
    return {a.x / b, a.y / b, a.z / b};
}

inline Vector3 operator+=(Vector3 &a, Vector3 b)
{
    return a = {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vector3 operator-=(Vector3 &a, Vector3 b)
{
    return a = {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Vector3 operator*=(Vector3 &a, Vector3 b)
{
    return a = {a.x * b.x, a.y * b.y, a.z * b.z};
}

inline Vector3 operator/=(Vector3 &a, Vector3 b)
{
    return a = {a.x / b.x, a.y / b.y, a.z / b.z};
}

inline Vector3 operator*=(Vector3 &a, float b)
{
    return a = {a.x * b, a.y * b, a.z * b};
}

inline Vector3 operator/=(Vector3 &a, float b)
{
    return a = {a.x / b, a.y / b, a.z / b};
}

inline float vector3_dot(Vector3 a, Vector3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float vector3_length(Vector3 a)
{
    return sqrt(vector3_dot(a, a));
}

inline Vector3 vector3_normalize(Vector3 a)
{
    return a / vector3_length(a);
}

inline Vector3 vector3_cross(Vector3 a, Vector3 b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

inline float vector2_dot(Vector2 a, Vector2 b)
{
    return a.x * b.x + a.y * b.y;
}

inline float vector2_length(Vector2 a)
{
    return sqrt(vector2_dot(a, a));
}

// For comparing distances, we don't need to take the square root
inline float vector2_cmp_distance(Vector2 a, Vector2 b)
{
    return vector2_dot(a - b, a - b);
}

inline Vector2 vector2_normalize(Vector2 a)
{
    return a / vector2_length(a);
}

inline Vector2 vector2_spread(float v)
{
    return {v, v};
}

inline Vector2 vector2_rotate(Vector2 a, float angle)
{
    float c = cos(angle), s = sin(angle);
    return {a.x * c - a.y * s, a.x * s + a.y * c};
}

inline Vector4 operator*(Vector4 a, Vector4 b)
{
    return {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
}

inline Vector4 operator*=(Vector4 &a, Vector4 b)
{
    return a = {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
}

inline Vector4 operator*(Vector4 a, float b)
{
    return {a.x * b, a.y * b, a.z * b, a.w * b};
}

inline Vector4 operator*=(Vector4 &a, float b)
{
    return a = {a.x * b, a.y * b, a.z * b, a.w * b};
}

inline Vector4 operator/(Vector4 a, float b)
{
    return {a.x / b, a.y / b, a.z / b, a.w / b};
}

inline Vector4 operator/=(Vector4 &a, float b)
{
    return a = {a.x / b, a.y / b, a.z / b, a.w / b};
}

inline Vector4 operator+(Vector4 a, Vector4 b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

inline Vector4 operator+=(Vector4 &a, Vector4 b)
{
    return a = {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

inline Vector4 operator-(Vector4 a, Vector4 b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

inline Vector4 operator-=(Vector4 &a, Vector4 b)
{
    return a = {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

inline float vector4_dot(Vector4 a, Vector4 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline float vector4_length(Vector4 a)
{
    return sqrt(vector4_dot(a, a));
}

inline Vector4 vector4_normalize(Vector4 a)
{
    return a / vector4_length(a);
}

inline Box3 box3_extrude_from_point_volume(Vector3 point, Vector3 extrude)
{
    extrude /= 2;

    Vector3 min = point;
    min.x -= extrude.x;
    min.y -= extrude.y;
    min.z -= extrude.z;
    Vector3 max = point;
    max.x += extrude.x;
    max.y += extrude.y;
    max.z += extrude.z;

    return {min, max};
}

inline Box3 box3_extrude_from_point(Vector3 point, Vector3 extrude)
{
    Vector3 min = point;
    min.x -= extrude.x;
    min.y -= extrude.y;
    min.z -= extrude.z;
    Vector3 max = point;
    max.x += extrude.x;
    max.y += extrude.y;
    max.z += extrude.z;

    return {min, max};
}

inline bool vector3_vs_box3(Vector3 vector, Box3 box)
{
    bool x_plane = vector.x >= box.min.x && vector.x <= box.max.x;
    bool y_plane = vector.y >= box.min.y && vector.y <= box.max.y;
    bool z_plane = vector.z >= box.min.z && vector.z <= box.max.z;
    return x_plane && y_plane && z_plane;
}

inline Box3 box3_translate(Box3 box, Vector3 vector)
{
    box.min.x += vector.x;
    box.min.y += vector.y;
    box.min.z += vector.z;

    box.max.x += vector.x;
    box.max.y += vector.y;
    box.max.z += vector.z;

    return box;
}

// NOTE: The "foot" of the player
inline Vector3 box3_base_midpoint(Box3 box)
{
    Vector3 pos = {};
    pos.x = (box.max.x - box.min.x) / 2;
    pos.y = box.min.y;
    pos.z = (box.max.z - box.min.z) / 2;

    return pos;
}

inline Vector3 box3_center(Box3 box)
{
    Vector3 pos = {};
    pos.x = (box.max.x - box.min.x) / 2;
    pos.y = (box.max.y - box.min.y) / 2;
    pos.z = (box.max.z - box.min.z) / 2;

    return pos;
}

// NOTE: The "eyes" of the player
inline Vector3 box3_eyes_midpoint(Box3 box)
{
    Vector3 pos = {};
    pos.x = (box.max.x - box.min.x) / 2;
    pos.y = box.max.y;
    pos.z = (box.max.z - box.min.z) / 2;

    return pos;
}

inline bool rect_vs_vector2(Rect rect, Vector2 point)
{
    return point.x >= rect.pos.x && point.y >= rect.pos.y &&
           point.x < (rect.pos.x + rect.siz.x) &&
           point.y < (rect.pos.y + rect.siz.y);
}

inline Rect at0(Rect rect)
{
    return {{0, 0}, rect.siz};
}

inline bool rect_vs_rect(Rect r1, Rect r2)
{
    return (r1.pos.x + r1.siz.x) > (r2.pos.x) &&
           (r1.pos.y + r1.siz.y) > (r2.pos.y) &&
           (r1.pos.x) < (r2.pos.x + r2.siz.x) &&
           (r1.pos.y) < (r2.pos.y + r2.siz.y);
}

inline Rect rect_center_rect(Rect parent, Rect child)
{
    return {parent.pos.x + (parent.siz.x - child.siz.x) / 2.0f,
            parent.pos.y + (parent.siz.y - child.siz.y) / 2.0f, child.siz.x,
            child.siz.y};
}

inline Vector2 rect_center(Rect r)
{
    return {r.pos.x + r.siz.x / 2.0f, r.pos.y + r.siz.y / 2.0f};
}

inline Rect rect_from_min_max(Vector2 min, Vector2 max)
{
    return {min.x, min.y, max.x - min.x, max.y - min.y};
}

inline Rect rect_shrink(Rect rect, Vector2 amount)
{
    rect.pos.x += amount.x;
    rect.pos.y += amount.y;
    rect.siz.x -= amount.x * 2;
    rect.siz.y -= amount.y * 2;

    return rect;
}

inline Vector2 rect_vs_rect_snap(Rect r, Rect along)
{
    float x_left = r.pos.x + r.siz.x - along.pos.x;
    float y_up = r.pos.y + r.siz.y - along.pos.y;

    float x_right = along.pos.x + along.siz.x - r.pos.x;
    float y_down = along.pos.y + along.siz.y - r.pos.y;

    float min_x = 0, min_y = 0;

    if (fabs(x_left) < fabs(x_right))
    {
        min_x = -x_left;
    }
    else
    {
        min_x = x_right;
    }

    if (fabs(y_up) < fabs(y_down))
    {
        min_y = -y_up;
    }
    else
    {
        min_y = y_down;
    }

    if (fabs(min_x) < fabs(min_y))
    {
        return {min_x, 0};
    }
    return {0, min_y};
}

inline Rect rect_and(Rect a, Rect b)
{
    float x1 = fmaxf(a.pos.x, b.pos.x);
    float y1 = fmaxf(a.pos.y, b.pos.y);
    float x2 = fminf(a.pos.x + a.siz.x, b.pos.x + b.siz.x);
    float y2 = fminf(a.pos.y + a.siz.y, b.pos.y + b.siz.y);

    return {x1, y1, fmaxf(0, x2 - x1), fmaxf(0, y2 - y1)};
}

inline bool ray3_vs_box3(Ray3 r, Box3 b, float max_distance, float *distance)
{
    r.direction = vector3_normalize(r.direction);
    Vector3 dirfrac;
    dirfrac.x = 1.0f / r.direction.x;
    dirfrac.y = 1.0f / r.direction.y;
    dirfrac.z = 1.0f / r.direction.z;
    float t1 = (b.min.x - r.origin.x) * dirfrac.x;
    float t2 = (b.max.x - r.origin.x) * dirfrac.x;
    float t3 = (b.min.y - r.origin.y) * dirfrac.y;
    float t4 = (b.max.y - r.origin.y) * dirfrac.y;
    float t5 = (b.min.z - r.origin.z) * dirfrac.z;
    float t6 = (b.max.z - r.origin.z) * dirfrac.z;

    float tmin = fmax(fmaxf(fminf(t1, t2), fminf(t3, t4)), fminf(t5, t6));
    float tmax = fmin(fminf(fmaxf(t1, t2), fmaxf(t3, t4)), fmaxf(t5, t6));

    if (tmax < 0)
    {
        *distance = tmax;
        return false;
    }

    if (tmin > tmax)
    {
        *distance = tmax;
        return false;
    }

    *distance = tmin;

    return true;
}

inline bool ray3_vs_horizontal_plane(Ray3 r, float y, float *t)
{
    r.direction = vector3_normalize(r.direction);

    if (r.origin.y < y)
    {
        if (r.direction.y > 0)
        {
            *t = (y - r.origin.y) / r.direction.y;
            return true;
        }
    }
    else if (r.origin.y > y)
    {
        if (r.direction.y < 0)
        {
            *t = (r.origin.y - y) / -r.direction.y;
            return true;
        }
    }

    return false;
}

inline Vector3 ray3_at(Ray3 r, float t)
{
    return r.origin + vector3_normalize(r.direction) * t;
}

struct Matrix4
{
    float values[16];

    static Matrix4 identity()
    {
        return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    }

    static Matrix4 perspective(float fov, float aspect, float near, float far)
    {
        float height = 1 / tanf(fov / 2);
        float width = height / aspect;

        return {width,
                0,
                0,
                0,
                0,
                height,
                0,
                0,
                0,
                0,
                -(near + far) / (near - far),
                1.0f,
                0,
                0,
                (2.0f * near * far) / (near - far),
                0};
    }

    static Matrix4 ortho(float left, float right, float bottom, float top,
                         float near, float far)
    {
        return {2.0f / (right - left),
                0,
                0,
                0,
                0,
                2.0f / (top - bottom),
                0,
                0,
                0,
                0,
                2.0f / (near - far),
                0,
                (left + right) / (left - right),
                (bottom + top) / (bottom - top),
                (near + far) / (near - far),
                1};
    }

    static Matrix4 look_at(Vector3 center, Vector3 eye, Vector3 up)
    {
        Vector3 z = vector3_normalize(eye - center);
        Vector3 x = vector3_normalize(vector3_cross(z, up));
        Vector3 y = vector3_cross(x, z);

        float xdot = -vector3_dot(x, center);
        float ydot = -vector3_dot(y, center);
        float zdot = -vector3_dot(z, center);

        return {x.x, y.x, -z.x, 0, x.y,  y.y,  -z.y,  0,
                x.z, y.z, -z.z, 0, xdot, ydot, -zdot, 1};
    }

    static Matrix4 translate(Vector3 center)
    {
        return {1, 0, 0, 0, 0,        1,        0,        0,
                0, 0, 1, 0, center.x, center.y, center.z, 1};
    }

    static Matrix4 scale(float factor)
    {
        return {factor, 0, 0, 0, 0, factor, 0, 0, 0, 0, factor, 0, 0, 0, 0, 1};
    }

    static Matrix4 scale_v(Vector3 factor)
    {
        return {factor.x, 0, 0,        0, 0, factor.y, 0, 0,
                0,        0, factor.z, 0, 0, 0,        0, 1};
    }

    static Matrix4 rotate_x(float angle)
    {
        float c = cos(-angle);
        float s = sin(-angle);
        return {
            1.0f, 0.0f, 0.0f, 0.0f, 0.0f, c,    s,    0.0f,
            0.0f, -s,   c,    0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        };
    }

    static Matrix4 rotate_y(float angle)
    {
        float c = cos(-angle);
        float s = sin(-angle);
        return {c, 0.0f, -s, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                s, 0.0f, c,  0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    }

    static Matrix4 rotate_z(float angle)
    {
        float c = cos(-angle);
        float s = sin(-angle);
        return {c,    s,    0.0f, 0.0f, -s,   c,    0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    }

    bool inverse(Matrix4 *out)
    {
        float *m = this->values;
        float inv[16], det;
        int i;

        inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] -
                 m[9] * m[6] * m[15] + m[9] * m[7] * m[14] +
                 m[13] * m[6] * m[11] - m[13] * m[7] * m[10];

        inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] +
                 m[8] * m[6] * m[15] - m[8] * m[7] * m[14] -
                 m[12] * m[6] * m[11] + m[12] * m[7] * m[10];

        inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] -
                 m[8] * m[5] * m[15] + m[8] * m[7] * m[13] +
                 m[12] * m[5] * m[11] - m[12] * m[7] * m[9];

        inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] +
                  m[8] * m[5] * m[14] - m[8] * m[6] * m[13] -
                  m[12] * m[5] * m[10] + m[12] * m[6] * m[9];

        inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] +
                 m[9] * m[2] * m[15] - m[9] * m[3] * m[14] -
                 m[13] * m[2] * m[11] + m[13] * m[3] * m[10];

        inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] -
                 m[8] * m[2] * m[15] + m[8] * m[3] * m[14] +
                 m[12] * m[2] * m[11] - m[12] * m[3] * m[10];

        inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] +
                 m[8] * m[1] * m[15] - m[8] * m[3] * m[13] -
                 m[12] * m[1] * m[11] + m[12] * m[3] * m[9];

        inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] -
                  m[8] * m[1] * m[14] + m[8] * m[2] * m[13] +
                  m[12] * m[1] * m[10] - m[12] * m[2] * m[9];

        inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] -
                 m[5] * m[2] * m[15] + m[5] * m[3] * m[14] +
                 m[13] * m[2] * m[7] - m[13] * m[3] * m[6];

        inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] +
                 m[4] * m[2] * m[15] - m[4] * m[3] * m[14] -
                 m[12] * m[2] * m[7] + m[12] * m[3] * m[6];

        inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] -
                  m[4] * m[1] * m[15] + m[4] * m[3] * m[13] +
                  m[12] * m[1] * m[7] - m[12] * m[3] * m[5];

        inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] +
                  m[4] * m[1] * m[14] - m[4] * m[2] * m[13] -
                  m[12] * m[1] * m[6] + m[12] * m[2] * m[5];

        inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] +
                 m[5] * m[2] * m[11] - m[5] * m[3] * m[10] -
                 m[9] * m[2] * m[7] + m[9] * m[3] * m[6];

        inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] -
                 m[4] * m[2] * m[11] + m[4] * m[3] * m[10] +
                 m[8] * m[2] * m[7] - m[8] * m[3] * m[6];

        inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] +
                  m[4] * m[1] * m[11] - m[4] * m[3] * m[9] -
                  m[8] * m[1] * m[7] + m[8] * m[3] * m[5];

        inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] -
                  m[4] * m[1] * m[10] + m[4] * m[2] * m[9] +
                  m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

        det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

        if (det == 0)
            return false;

        det = 1.0 / det;

        for (i = 0; i < 16; i++)
            out->values[i] = inv[i] * det;

        return true;
    }
};

inline Vector4 operator*(Matrix4 a, Vector4 b)
{
    float data[4] = {b.x, b.y, b.z, b.w};
    float ret[4] = {};

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            ret[i] += a.values[j * 4 + i] * data[j];
        }
    }

    return {ret[0], ret[1], ret[2], ret[3]};
}

inline Matrix4 operator*(Matrix4 a, Matrix4 b)
{
    Matrix4 result = {};

    result.values[0] += a.values[0] * b.values[0];
    result.values[0] += a.values[4] * b.values[1];
    result.values[0] += a.values[8] * b.values[2];
    result.values[0] += a.values[12] * b.values[3];

    result.values[1] += a.values[1] * b.values[0];
    result.values[1] += a.values[5] * b.values[1];
    result.values[1] += a.values[9] * b.values[2];
    result.values[1] += a.values[13] * b.values[3];

    result.values[2] += a.values[2] * b.values[0];
    result.values[2] += a.values[6] * b.values[1];
    result.values[2] += a.values[10] * b.values[2];
    result.values[2] += a.values[14] * b.values[3];

    result.values[3] += a.values[3] * b.values[0];
    result.values[3] += a.values[7] * b.values[1];
    result.values[3] += a.values[11] * b.values[2];
    result.values[3] += a.values[15] * b.values[3];

    result.values[4] += a.values[0] * b.values[4];
    result.values[4] += a.values[4] * b.values[5];
    result.values[4] += a.values[8] * b.values[6];
    result.values[4] += a.values[12] * b.values[7];

    result.values[5] += a.values[1] * b.values[4];
    result.values[5] += a.values[5] * b.values[5];
    result.values[5] += a.values[9] * b.values[6];
    result.values[5] += a.values[13] * b.values[7];

    result.values[6] += a.values[2] * b.values[4];
    result.values[6] += a.values[6] * b.values[5];
    result.values[6] += a.values[10] * b.values[6];
    result.values[6] += a.values[14] * b.values[7];

    result.values[7] += a.values[3] * b.values[4];
    result.values[7] += a.values[7] * b.values[5];
    result.values[7] += a.values[11] * b.values[6];
    result.values[7] += a.values[15] * b.values[7];

    result.values[8] += a.values[0] * b.values[8];
    result.values[8] += a.values[4] * b.values[9];
    result.values[8] += a.values[8] * b.values[10];
    result.values[8] += a.values[12] * b.values[11];

    result.values[9] += a.values[1] * b.values[8];
    result.values[9] += a.values[5] * b.values[9];
    result.values[9] += a.values[9] * b.values[10];
    result.values[9] += a.values[13] * b.values[11];

    result.values[10] += a.values[2] * b.values[8];
    result.values[10] += a.values[6] * b.values[9];
    result.values[10] += a.values[10] * b.values[10];
    result.values[10] += a.values[14] * b.values[11];

    result.values[11] += a.values[3] * b.values[8];
    result.values[11] += a.values[7] * b.values[9];
    result.values[11] += a.values[11] * b.values[10];
    result.values[11] += a.values[15] * b.values[11];

    result.values[12] += a.values[0] * b.values[12];
    result.values[12] += a.values[4] * b.values[13];
    result.values[12] += a.values[8] * b.values[14];
    result.values[12] += a.values[12] * b.values[15];

    result.values[13] += a.values[1] * b.values[12];
    result.values[13] += a.values[5] * b.values[13];
    result.values[13] += a.values[9] * b.values[14];
    result.values[13] += a.values[13] * b.values[15];

    result.values[14] += a.values[2] * b.values[12];
    result.values[14] += a.values[6] * b.values[13];
    result.values[14] += a.values[10] * b.values[14];
    result.values[14] += a.values[14] * b.values[15];

    result.values[15] += a.values[3] * b.values[12];
    result.values[15] += a.values[7] * b.values[13];
    result.values[15] += a.values[11] * b.values[14];
    result.values[15] += a.values[15] * b.values[15];

    return result;
}

inline Matrix4 operator*=(Matrix4 &a, Matrix4 b)
{
    return a = a * b;
}

inline Vector3 operator*(Matrix4 a, Vector3 b)
{
    float data[4] = {b.x, b.y, b.z, 1.0};
    float ret[4] = {};

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            ret[i] += a.values[j * 4 + i] * data[j];
        }
    }

    return {ret[0], ret[1], ret[2]};
}

// Conversion Operations

inline Vector2i vector2_to_vector2i(Vector2 v)
{
    return {(int32_t)v.x, (int32_t)v.y};
}

inline Vector2 vector2i_to_vector2(Vector2i v)
{
    return {(float)v.x, (float)v.y};
}

// Bounds Operations

template <class T> T clamp(T value, T min, T max)
{
    return value < min ? min : value > max ? max : value;
}

template <class T> T min(T a, T b)
{
    return a < b ? a : b;
}

template <class T> T max(T a, T b)
{
    return a > b ? a : b;
}

inline float snap(float value, float snap)
{
    return round(value / snap) * snap;
}

// Interpolation Operations

inline float slerp(float a, float b, float decay, float dt)
{
    return a + (b - a) * (1.0f - exp(-decay * dt));
}

inline Vector2 slerp(Vector2 a, Vector2 b, float decay, float dt)
{
    return a + (b - a) * (1.0f - exp(-decay * dt));
}

inline Vector3 slerp(Vector3 a, Vector3 b, float decay, float dt)
{
    return a + (b - a) * (1.0f - exp(-decay * dt));
}

inline Vector4 slerp(Vector4 a, Vector4 b, float decay, float dt)
{
    return a + (b - a) * (1.0f - exp(-decay * dt));
}

inline float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

inline Vector2 lerp(Vector2 a, Vector2 b, float t)
{
    return a + (b - a) * t;
}

inline Vector3 lerp(Vector3 a, Vector3 b, float t)
{
    return a + (b - a) * t;
}

inline Vector4 lerp(Vector4 a, Vector4 b, float t)
{
    return a + (b - a) * t;
}
