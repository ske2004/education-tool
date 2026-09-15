#include "camera.hpp"
#include <cassert>

static Matrix4 rotation_matrix(Camera *camera)
{
    float yaw_rad = camera->yaw * MATH_DEG_TO_RAD;
    float pitch_rad = camera->pitch * MATH_DEG_TO_RAD;
    return Matrix4::rotate_y(-yaw_rad) * Matrix4::rotate_x(pitch_rad);
}

static Matrix4 make_vp(Camera *camera)
{
    Vector3 eye = rotation_matrix(camera) * Vector3{0.0f, 0.0f, -1.0f};
    Vector3 p = camera->position;
    Matrix4 view = Matrix4::look_at(p, p + eye, {0.0f, 1.0f, 0.0f});
    Matrix4 proj = Matrix4::perspective(camera->fov_deg * MATH_DEG_TO_RAD,
                                        camera->aspect, 0.1f, 1000.0f);

    return proj * view;
}

Camera Camera::init(float fov_deg)
{
    Camera result = {};
    result.fov_deg = fov_deg;
    result.vp = make_vp(&result);
    return result;
}

void Camera::set_aspect(float aspect)
{
    this->aspect = aspect;
    this->vp = make_vp(this);
}

void Camera::rotate(float yaw, float pitch)
{
    this->yaw += yaw;
    this->pitch += pitch;

    // Normalize
    this->yaw = fmod(this->yaw, 360.0f);
    this->pitch = fmod(this->pitch, 360.0f);

    // Restrict pitch
    if (this->pitch > 90.0f - MATH_EPSILON)
    {
        this->pitch = 90.0f - MATH_EPSILON;
    }
    if (this->pitch < -(90.0f - MATH_EPSILON))
    {
        this->pitch = -(90.0f - MATH_EPSILON);
    }

    this->vp = make_vp(this);
}

void Camera::rotate_around(Vector3 point, float yaw, float pitch)
{
    Vector3 pos = Matrix4::rotate_y(-yaw * MATH_DEG_TO_RAD) *
                  Matrix4::rotate_x(pitch * MATH_DEG_TO_RAD) *
                  (this->position - point);
    this->position = pos;

    rotate(yaw, pitch);
}

Ray3 Camera::ray()
{
    Vector3 eye = rotation_matrix(this) * Vector3{0.0f, 0.0f, 1.0f};
    return {this->position, eye};
}

Ray3 Camera::screen_to_world_ray(Vector2 screen_pos, Vector2 viewport)
{
    // Convert screen position to normalized device coordinates (NDC)
    float ndc_x = (2.0f * screen_pos.x) / viewport.x - 1.0f;
    float ndc_y = 1.0f - (2.0f * screen_pos.y) / viewport.y;

    // Convert NDC to camera space
    Vector3 eye = rotation_matrix(this) * Vector3{0.0f, 0.0f, -1.0f};
    Matrix4 view = Matrix4::look_at({0, 0, 0}, eye, {0.0f, 1.0f, 0.0f});
    Matrix4 proj = Matrix4::perspective(this->fov_deg * MATH_DEG_TO_RAD,
                                        aspect, 0.1f, 1000.0f);
    Matrix4 inv_vp = {};
    bool ok = (proj * view).inverse(&inv_vp);
    assert(ok);
    Vector4 clip_coords = Vector4{ndc_x, ndc_y, -1.0f, 1.0f};

    Vector4 eye_coords = inv_vp * clip_coords;
    Vector3 camera_ray = Vector3{eye_coords.x, eye_coords.y, eye_coords.z};

    // Create the camera ray
    return {this->position, camera_ray};
}

void Camera::move(float sideways, float upward, float forward)
{
    this->position +=
        (Matrix4::rotate_y(-this->yaw * MATH_DEG_TO_RAD) * Vector3{sideways, upward, forward});
    this->vp = make_vp(this);
}
