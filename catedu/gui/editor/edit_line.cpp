#include "edit_line.hpp"
#include "catedu/core/math/brezenham.hpp"
#include "catedu/genobj/grid.hpp"
#include "catedu/genobj/road.hpp"
#include "catedu/genobj/wall.hpp"
#include "catedu/scene/world.hpp"

void EditLine::update(Dispatcher &disp, Input &input, Camera &camera,
                      Vector2 viewport, Object::Type type)
{
    Ray3 pointer_ray = camera.screen_to_world_ray(
        input.mouse_pos, viewport);

    float t;
    ray3_vs_horizontal_plane(pointer_ray, -0.5, &t);

    Vector3 at = ray3_at(pointer_ray, t);
    Vector2 pointer = {floorf(at.x), floorf(at.z)};

    cursor = pointer;

    if (input.k[INPUT_MB_LEFT].pressed)
    {
        started = true;
    }
    if (input.k[INPUT_MB_LEFT].held)
    {
        pointer_end = pointer;
    }
    else if (!input.k[INPUT_MB_LEFT].released)
    {
        pointer_start = pointer;
        pointer_end = pointer;
        started = false;
    }

    if (started)
    {
        if (input.k[INPUT_MB_LEFT].released)
        {
            brezenham(pointer_start.x, pointer_start.y, pointer_end.x,
                      pointer_end.y, [&](int x, int y) {
                          Object obj = {};
                          obj.type = type;
                          obj.x = x;
                          obj.y = y;
                          disp.place_object(obj);
                      });
        }
    }
}

void EditLine::render(Renderer &renderer, Dispatcher &disp,
                      GenResources &gen_resources, Object::Type type)
{
    // TODO: We shouldn't  do this, but I need to so that the preview doesn't
    // show overlapping objects. I use this to check if the object can be
    // placed, I should probably do this in a better way.
    Place collisiontest = Place::create();

    auto preview = [&](int x, int y) {
        Object obj = {};
        obj.type = type;
        obj.x = x;
        obj.y = y;

        if (collisiontest.can_place_objtype(type, x, y) &&
            disp.world.current->can_place_objtype(type, x, y))
        {
            collisiontest.place_object(obj);

            GeneratedObject obj;

            switch (type)
            {
            case Object::Type::wall:
                obj = genmesh_generate_wall();
                break;
            case Object::Type::road:
                obj = genmesh_generate_road();
                break;
            default:
                assert(false);
                break;
            }

            genobj_render_object(renderer, gen_resources, obj,
                                 Matrix4::translate({(float)x, 0, (float)y}));
        }
    };
    brezenham(pointer_start.x, pointer_start.y, pointer_end.x, pointer_end.y,
              preview);

    collisiontest.destroy();

    GeneratedObject grid = genmesh_generate_grid(16, 16);
    genobj_render_object(renderer, gen_resources, grid,
                         Matrix4::translate({cursor.x, 0, cursor.y}));
}
