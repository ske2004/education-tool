#include "camera.hpp"

EditorCamera EditorCamera::create()
{
    EditorCamera camera = {};
    camera.cam = Camera::init(45);
    camera.cam.rotate(0, -45);
    camera.zoom_target = 6;
    return camera;
}

void EditorCamera::lockin(Vector3 pos, float rotation_deg)
{
    cam.move(0, -(zoom * zoom), (zoom * zoom));
    cam.yaw = rotation_deg;
    zoom_target = 4;
    cam.position = pos;
    cam.move(0, 0, 0);
    cam.move(0, (zoom * zoom), -(zoom * zoom));
}

void EditorCamera::follow(Vector3 pos, float rotation_deg, float zoom_target)
{
    cam.move(0, -(zoom * zoom), (zoom * zoom));
    cam.yaw = slerp(cam.yaw, rotation_deg, 5, sapp_frame_duration());
    this->zoom_target = zoom_target;
    cam.position = slerp(cam.position, pos, 5, sapp_frame_duration());
    cam.move(0, 0, 0);
    cam.move(0, (zoom * zoom), -(zoom * zoom));
}

void EditorCamera::update(Vector2i window_size)
{
    cam.set_aspect((float)window_size.x / (float)window_size.y);

    cam.move(0, -(zoom * zoom), (zoom * zoom));
    zoom = slerp(zoom, zoom_target, 5, sapp_frame_duration());
    cam.move(0, (zoom * zoom), -(zoom * zoom));
}

void EditorCamera::handle_controls(Input &input)
{
    cam.move(0, -(zoom * zoom), (zoom * zoom));

    if (input.k[INPUT_MB_MIDDLE].held)
    {
        if (input.k[INPUT_MB_RIGHT].held)
        {
            cam.rotate(input.mouse_delta.x / 5, 0);
        }
        else
        {
            cam.move(-input.mouse_delta.x / 10, 0, input.mouse_delta.y / 10);
        }
    }

    zoom_target -= input.mouse_wheel;
    zoom_target = clamp<float>(zoom_target, 2, 8);

    cam.move(0, (zoom * zoom), -(zoom * zoom));
}
