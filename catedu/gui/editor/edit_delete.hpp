#pragma once

#include "catedu/genobj/render.hpp"
#include "catedu/gui/editor/dispatcher.hpp"
#include "catedu/ui/user.hpp"

struct EditDelete
{
    bool has_target;
    RectI target_bounds;

    void update(Dispatcher &disp, Input &input, Camera &camera);
    void render(Renderer &renderer, GenResources &gen_resources);
};
