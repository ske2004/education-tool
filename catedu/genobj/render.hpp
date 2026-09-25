#pragma once
#include "catedu/rendering/3d/pbr.hpp"
#include "generator.hpp"

struct ResourceSpec;

struct GenResources
{
    Model box;
    Model box_shaded;
    Model box_window;
    // Magenta checker cube for props with an unknown ID.
    Model box_invalid;
    // Red outline cube marking a tile that can't be placed on.
    Model selector;
    Model grass_floor;
    ResourceSpec *spec;
};

void genobj_render_object(Renderer &renderer, const GenResources &resources,
                          const GeneratedObject &object,
                          Matrix4 matrix = Matrix4::identity(), float time_of_day = 12.0f);

void genobj_render_model(Renderer &renderer, const Model &model,
                         Matrix4 matrix, float time_of_day = 12.0f);

void genobj_render_prop(Renderer &renderer, const GenResources &resources,
                        const char *id, Matrix4 matrix = Matrix4::identity(),
                        float time_of_day = 12.0f);
