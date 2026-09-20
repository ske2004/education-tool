#pragma once

#include "catedu/genobj/render.hpp"
#include "catedu/rendering/3d/pbr.hpp"
#include "catedu/rendering/resource_spec.hpp"
#include "world.hpp"

GenResources get_genres(ResourceSpec &resources);
void render_place(Place &place, Renderer &renderer, ResourceSpec &resources, float time_of_day);
