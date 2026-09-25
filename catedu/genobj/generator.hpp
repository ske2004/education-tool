#pragma once
#include "catedu/core/math/math.hpp"
#include "catedu/rendering/color.hpp"

enum
{
    COMPONENT_COUNT = 128
};

struct GeneratedObjectComponent
{
    Box3 box;
    Color color;
    bool shaded;
    bool window;
};

struct GeneratedObject
{
    GeneratedObjectComponent components[COMPONENT_COUNT];
    size_t num_components;

    void push(GeneratedObjectComponent);
    void push_colored_box(Box3 box, Color color, bool shaded = false);
    void push_window_box(Box3 box);
};
