#include "render_world.hpp"
#include <math.h>
#include "catedu/genobj/building.hpp"
#include "catedu/genobj/ground.hpp"
#include "catedu/genobj/player.hpp"
#include "catedu/genobj/render.hpp"
#include "catedu/genobj/road.hpp"
#include "catedu/genobj/npc.hpp"
#include "catedu/genobj/animal.hpp"
#include "catedu/genobj/tree.hpp"
#include "catedu/genobj/wall.hpp"
#include "catedu/genobj/item.hpp"
#include "catedu/genobj/water.hpp"
#include "catedu/genobj/high_grass.hpp"
#include "catedu/genobj/prop.hpp"
#include "catedu/genobj/bridge.hpp"
#include "catedu/genobj/castle.hpp"

GenResources get_genres(ResourceSpec &resources)
{
    GenResources result = {};
    result.box =
        resources.models.get_assert(resources.find_model_by_name("cube")).model;
    result.box_shaded =
        resources.models.get_assert(resources.find_model_by_name("cube_shaded"))
            .model;
    result.box_window =
        resources.models.get_assert(resources.find_model_by_name("cube_window"))
            .model;
    return result;
}

void show_backdrop(Renderer &renderer, ResourceSpec &resources, float time_of_day)
{
    Params vs_params = {};

    vs_params.model =
        Matrix4::translate(renderer.camera.position) * Matrix4::scale(32);
    vs_params.lightness = 1.0f;
    
    // Calculate sky color
    float t = time_of_day / 24.0f;
    float angle = t * 3.14159265f * 2.0f;
    float day_intensity = sinf(angle - 3.14159265f / 2.0f) * 0.5f + 0.5f;
    
    Vector4 day_sky = {0.5f, 0.7f, 1.0f, 1.0f};
    Vector4 night_sky = {0.02f, 0.05f, 0.1f, 1.0f};
    Vector4 sunset_sky = {0.8f, 0.4f, 0.2f, 1.0f};
    
    Vector4 sky_color;
    if (day_intensity > 0.5f) {
        float f = (day_intensity - 0.5f) * 2.0f;
        sky_color = {
            sunset_sky.x + (day_sky.x - sunset_sky.x) * f,
            sunset_sky.y + (day_sky.y - sunset_sky.y) * f,
            sunset_sky.z + (day_sky.z - sunset_sky.z) * f, 1.0f};
    } else {
        float f = day_intensity * 2.0f;
        sky_color = {
            night_sky.x + (sunset_sky.x - night_sky.x) * f,
            night_sky.y + (sunset_sky.y - night_sky.y) * f,
            night_sky.z + (sunset_sky.z - night_sky.z) * f, 1.0f};
    }
    
    vs_params.color_mul = sky_color;
    // Set dummy lighting to avoid uninitialized memory issues in shader
    vs_params.light_dir = {0, -1, 0};
    vs_params.light_color = {1, 1, 1, 1};
    vs_params.ambient_color = {1, 1, 1, 1};
    
    renderer.render_model(
        resources.models.get_assert(resources.find_model_by_name("skybox"))
            .model,
        vs_params);
}

void render_place(Place &place, Renderer &renderer, ResourceSpec &resources, float time_of_day)
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
        case Object::Type::npc:
            mesh = genmesh_generate_npc();
            break;
        case Object::Type::item:
            mesh = genmesh_generate_item();
            break;
        case Object::Type::animal:
            mesh = genmesh_generate_animal();
            break;
        case Object::Type::water:
            mesh = genmesh_generate_water();
            break;
        case Object::Type::high_grass:
            mesh = genmesh_generate_high_grass();
            break;
        case Object::Type::prop:
            mesh = genmesh_generate_prop(object.id);
            break;
        case Object::Type::castle:
            mesh = genmesh_generate_castle(object.floors);
            break;
        case Object::Type::bridge:
            mesh = genmesh_generate_bridge();
            break;
        }

        genobj_render_object(
            renderer, gen_resources, mesh,
            Matrix4::translate({(float)object.x, (float)object.z, (float)object.y}), time_of_day);
    }

    if (place.interior)
    {
        genobj_render_object(renderer, gen_resources,
                             genmesh_generate_ground(true), Matrix4::identity(), time_of_day);
    }
    else
    {
        show_backdrop(renderer, resources, time_of_day);
        genobj_render_object(renderer, gen_resources,
                             genmesh_generate_ground(false), Matrix4::identity(), time_of_day);
    }
}
