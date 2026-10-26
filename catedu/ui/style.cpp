#include "style.hpp"

UiStyle &UiStyle::background(UiBrush brush)
{
    nx_background = brush;
    return *this;
}
UiStyle &UiStyle::border(UiBrush brush)
{
    nx_border = brush;
    return *this;
}
UiStyle &UiStyle::color(UiBrush brush)
{
    nx_color = brush;
    return *this;
}
UiStyle &UiStyle::border_size(float size)
{
    nx_border_size = size;
    return *this;
}
UiStyle &UiStyle::margin(float size)
{
    nx_margin = size;
    return *this;
}
UiStyle &UiStyle::padding(float size)
{
    nx_padding = size;
    return *this;
}

UiStyle &UiStyle::with_border_radius(float border_radius)
{
    this->border_radius = border_radius;
    return *this;
}

UiStyle &UiStyle::with_squircle_rate(float squircle_rate)
{
    this->squircle_rate = squircle_rate;
    return *this;
}

UiStyle &UiStyle::with_z_index(int z_index)
{
    this->z_index = z_index;
    return *this;
}
