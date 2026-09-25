#include "edit_delete.hpp"
#include "catedu/genobj/bounds.hpp"
#include "catedu/genobj/generator.hpp"
#include "catedu/genobj/render.hpp"
#include "catedu/sys/input.hpp"

void EditDelete::update(Dispatcher &disp, Input &input, Camera &camera,
                        Vector2 viewport)
{
    Ray3 pointer_ray = camera.screen_to_world_ray(
        input.mouse_pos, viewport);

    float t;
    ray3_vs_horizontal_plane(pointer_ray, 0.0, &t);

    Vector3 at = ray3_at(pointer_ray, t);
    Vector2 pointer = {floorf(at.x), floorf(at.z)};

    // The player can stand on a road, so target it first.
    Object *obj = disp.world.current->get_object_at(pointer.x, pointer.y,
                                                    Object::Type::player);
    if (!obj)
    {
        obj = disp.world.current->get_object_at(pointer.x, pointer.y);
    }

    if (obj)
    {
        has_target = true;
        target_bounds = disp.world.current->object_bounds(*obj);

        if (input.k[INPUT_MB_LEFT].held)
        {
            disp.remove_object(obj);
        }
    }
    else
    {
        has_target = false;
    }
}

void EditDelete::render(Renderer &renderer, GenResources &gen_resources)
{
    if (has_target)
    {
        GeneratedObject obj =
            genmesh_generate_bounds(target_bounds, Color::hex(0xFF0000FF));

        genobj_render_object(renderer, gen_resources, obj);
    }
}
