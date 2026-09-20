#pragma once
#include "catedu/gui/editor/dispatcher.hpp"
#include <catedu/genobj/render.hpp>
#include <catedu/rendering/3d/camera.hpp>
#include <catedu/ui/user.hpp>

struct EditBasic
{
    Vector2 cursor;
    bool valid;
    char prop_id[32] = "prop";

    void update(Dispatcher &disp, Input &input, Camera &camera,
                Vector2 viewport, Object::Type type);
    void render(Renderer &renderer, GenResources &gen_resources,
                Object::Type type);
};
