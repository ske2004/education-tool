///
/// NOTE: For editing walls and roads
///

#pragma once

#include "catedu/genobj/render.hpp"
#include "catedu/gui/editor/dispatcher.hpp"
#include "catedu/rendering/3d/camera.hpp"
#include "catedu/ui/user.hpp"

struct EditLine
{
    bool started;
    Vector2 pointer_start;
    Vector2 pointer_end;
    Vector2 cursor;

    void update(Dispatcher &disp, Input &input, Camera &camera,
                Object::Type type);
    void render(Renderer &renderer, Dispatcher &disp,
                GenResources &gen_resources, Object::Type type);
};
