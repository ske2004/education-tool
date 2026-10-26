#pragma once

#include "catedu/ui/rendering/core.hpp"
struct UiStyle
{
    UiBrush nx_background;
    UiBrush nx_border;
    UiBrush nx_color;
    float nx_border_size;
    float nx_margin;
    float nx_padding;
    float border_radius = 0.5f;
    float squircle_rate = 0.9f;
    int z_index = 0;

    UiStyle &background(UiBrush brush);

    UiStyle &border(UiBrush brush);

    UiStyle &color(UiBrush brush);

    UiStyle &border_size(float size);

    UiStyle &margin(float size);

    UiStyle &padding(float size);

    UiStyle &with_border_radius(float border_radius);

    UiStyle &with_squircle_rate(float squircle_rate);

    UiStyle &with_z_index(int z_index);
};

struct UiStyleHandler
{
};
