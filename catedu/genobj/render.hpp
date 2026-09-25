#pragma once
#include "catedu/rendering/3d/pbr.hpp"
#include "generator.hpp"

struct GenResources
{
    Model box;
    Model box_shaded;
    Model box_window;
};

void genobj_render_object(Renderer &renderer, const GenResources &resources,
                          const GeneratedObject &object,
                          Matrix4 matrix = Matrix4::identity(), float time_of_day = 12.0f);
