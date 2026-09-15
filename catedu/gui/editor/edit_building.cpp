#include "edit_building.hpp"
#include <catedu/genobj/building.hpp>
#include <catedu/genobj/grid.hpp>

void EditBuilding::update(Dispatcher &disp, Input &input, Camera &camera,
                          Vector2 viewport)
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
        disp.place_object({Object::Type::building, floors, x, y});
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
                if (obj->type == Object::Type::building)
                {
                    disp.world.script->acquire_place_event(obj->place);
                    disp.enter_place(obj);
                }
            }
        }
    }

    valid = disp.world.current->can_place_building(floors, x, y);
}

void EditBuilding::render(Renderer &renderer, GenResources &gen_resources)
{
    if (valid)
    {
        GeneratedObject building = genmesh_generate_building(floors);
        genobj_render_object(renderer, gen_resources, building,
                             Matrix4::translate({x, 0, y}));
    }

    GeneratedObject grid = genmesh_generate_grid(16, 16);
    genobj_render_object(renderer, gen_resources, grid,
                         Matrix4::translate({x, 0, y}));
}
