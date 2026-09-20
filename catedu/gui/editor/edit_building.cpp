#include "edit_building.hpp"
#include <catedu/genobj/building.hpp>
#include <catedu/genobj/castle.hpp>
#include <catedu/genobj/grid.hpp>

void EditBuilding::update(Dispatcher &disp, Input &input, Camera &camera,
                          Vector2 viewport, Object::Type type)
{
    Ray3 pointer_ray = camera.screen_to_world_ray(
        input.mouse_pos, viewport);

    float t;
    ray3_vs_horizontal_plane(pointer_ray, 0, &t);

    Vector3 at = ray3_at(pointer_ray, t);
    Vector2 pointer = {floorf(at.x), floorf(at.z)};

    if (placing)
    {
        floors += input.mouse_wheel;

        // HACK: Prevent the zoom-in-out in the camera handler.
        input.mouse_wheel = 0;
    }

    floors = clamp(floors, 1, 10);

    if (!placing)
    {
        x = pointer.x;
        y = pointer.y;
        floors = 1;
    }

    if (input.k[INPUT_MB_LEFT].pressed && placing)
    {
        disp.place_object({type, floors, x, y});
        Place *prev = disp.world.current;
        if (Object *obj = disp.world.current->get_object_at(x, y)) {
            disp.enter_place(obj);
            disp.world.current = prev;
        }
        placing = false;
    }
    else if (input.k[INPUT_MB_LEFT].pressed)
    {
        placing = disp.world.current->can_place_building(floors, x, y);
        if (!placing)
        {
            if (Object *obj = disp.world.current->get_object_at(x, y);
                obj != nullptr)
            {
                if (obj->type == Object::Type::building || obj->type == Object::Type::castle)
                {
                    disp.world.script->acquire_place_event(obj->place);
                    disp.enter_place(obj);
                }
            }
        }
    }
    else if (input.k[INPUT_MB_RIGHT].pressed)
    {
        placing = false;
    }

    valid = disp.world.current->can_place_building(floors, x, y);
}

void EditBuilding::render(Renderer &renderer, GenResources &gen_resources, Object::Type type)
{
    if (valid)
    {
        GeneratedObject obj;
        if (type == Object::Type::castle) {
            obj = genmesh_generate_castle(floors);
        } else {
            obj = genmesh_generate_building(floors);
        }

        genobj_render_object(renderer, gen_resources, obj,
                             Matrix4::translate({(float)x, 0, (float)y}));
    }

    if (placing)
    {
        GeneratedObject grid = genmesh_generate_grid(16, 16);
        genobj_render_object(renderer, gen_resources, grid,
                             Matrix4::translate({x, 0, y}));
    }
}
