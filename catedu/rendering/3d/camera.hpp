///
/// BRIEF: First-person camera controller.
/// All angles (fov, yaw, pitch) are stored in degrees.
///

#pragma once
#include "catedu/core/math/math.hpp"

struct Camera
{
    float fov_deg;
    float aspect;
    float yaw, pitch; // degrees
    Vector3 position;
    Matrix4 vp;

    static Camera init(float fov_deg);
    void set_aspect(float aspect);
    void rotate(float yaw, float pitch);
    void rotate_around(Vector3 point, float yaw, float pitch);
    Ray3 ray();
    Ray3 screen_to_world_ray(Vector2 screen_pos, Vector2 viewport);
    void move(float sideways, float upward, float forward);
};
