#include "render_world.hpp"
#include "catedu/genobj/building.hpp"
#include "catedu/genobj/ground.hpp"
#include "catedu/genobj/player.hpp"
#include "catedu/genobj/render.hpp"
#include "catedu/genobj/road.hpp"
#include "catedu/genobj/tree.hpp"
#include "catedu/genobj/wall.hpp"

GenResources get_genres(ResourceSpec &resources)
{
    GenResources result = {};
    result.box =
        resources.models.get_assert(resources.find_model_by_name("cube")).model;
    result.box_shaded =
        resources.models.get_assert(resources.find_model_by_name("cube_shaded"))
            .model;
    return result;
}

void show_backdrop(Renderer &renderer, ResourceSpec &resources)
{
    Params vs_params;

    vs_params.model =
        Matrix4::translate(renderer.camera.position) * Matrix4::scale(32);
    vs_params.lightness = 1.0f;
    vs_params.color_mul = {1.0f, 1.0f, 1.0f, 1.0f};
    renderer.render_model(
        resources.models.get_assert(resources.find_model_by_name("skybox"))
            .model,
        vs_params);
}

void render_place(Place &place, Renderer &renderer, ResourceSpec &resources)
{
    GenResources gen_resources = get_genres(resources);

    for (auto &object : iter(place.objects))
    {
        GeneratedObject mesh = {};

        switch (object.type)
        {
        case Object::Type::road:
            mesh = genmesh_generate_road();
            break;
        case Object::Type::building:
            mesh = genmesh_generate_building(object.floors);
            break;
        case Object::Type::player:
            mesh = genmesh_generate_player();
            break;
        case Object::Type::wall:
            mesh = genmesh_generate_wall();
            break;
        case Object::Type::tree:
            mesh = genmesh_generate_tree();
            break;
        }

        genobj_render_object(
            renderer, gen_resources, mesh,
            Matrix4::translate({(float)object.x, 0, (float)object.y}));
    }

    if (place.interior)
    {
        genobj_render_object(renderer, gen_resources,
                             genmesh_generate_ground(true));
    }
    else
    {
        show_backdrop(renderer, resources);
        genobj_render_object(renderer, gen_resources,
                             genmesh_generate_ground(false));
    }
}
