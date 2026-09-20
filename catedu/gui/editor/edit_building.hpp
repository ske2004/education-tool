#pragma once
#include "catedu/genobj/render.hpp"
#include "catedu/gui/editor/dispatcher.hpp"
#include "catedu/rendering/3d/camera.hpp"
#include "catedu/ui/user.hpp"

struct EditBuilding
{
    bool placing;
    float x;
    float y;
    int floors;
    bool valid;

    void update(Dispatcher &disp, Input &input, Camera &camera,
                Vector2 viewport, Object::Type type = Object::Type::building);
    void render(Renderer &renderer, GenResources &gen_resources, Object::Type type = Object::Type::building);
};
