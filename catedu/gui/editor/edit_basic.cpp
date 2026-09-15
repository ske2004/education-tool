#include "edit_basic.hpp"
#include "catedu/genobj/tree.hpp"
#include "catedu/scene/world.hpp"
#include <catedu/genobj/grid.hpp>
#include <catedu/genobj/player.hpp>
#include <catedu/genobj/road.hpp>
#include <catedu/genobj/wall.hpp>

void EditBasic::update(Dispatcher &disp, Input &input, Camera &camera,
                       Vector2 viewport, Object::Type type)
{
    Ray3 pointer_ray = camera.screen_to_world_ray(
        input.mouse_pos, viewport);

    float t;
    ray3_vs_horizontal_plane(pointer_ray, 0.0, &t);

    Vector3 at = ray3_at(pointer_ray, t);
    cursor = {floorf(at.x), floorf(at.z)};

    if (input.k[INPUT_MB_LEFT].pressed)
    {
        disp.place_object({type, 0, cursor.x, cursor.y});
    }

    valid = disp.world.current->can_place_objtype(type, cursor.x, cursor.y);
}

void EditBasic::render(Renderer &renderer, GenResources &gen_resources,
                       Object::Type type)
{
    if (valid)
    {
        GeneratedObject obj;

        switch (type)
        {
        case Object::Type::player:
            obj = genmesh_generate_player();
            break;
        case Object::Type::tree:
            obj = genmesh_generate_tree();
            break;
        default:
            assert(false);
            break;
        }

        genobj_render_object(renderer, gen_resources, obj,
                             Matrix4::translate({cursor.x, 0, cursor.y}));
    }

    GeneratedObject grid = genmesh_generate_grid(16, 16);
    genobj_render_object(renderer, gen_resources, grid,
                         Matrix4::translate({cursor.x, 0, cursor.y}));
}
