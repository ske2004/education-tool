#include "generator.hpp"
#include <cassert>

void GeneratedObject::push(GeneratedObjectComponent component)
{
    assert(num_components < COMPONENT_COUNT);
    components[num_components++] = component;
}

void GeneratedObject::push_colored_box(Box3 box, Color color, bool shaded)
{
    GeneratedObjectComponent component = {};
    component.box = box;
    component.color = color;
    component.shaded = shaded;

    push(component);
}

void GeneratedObject::push_window_box(Box3 box)
{
    GeneratedObjectComponent component = {};
    component.box = box;
    component.color = Color::hex(0xFFFFFFFF);
    component.window = true;

    push(component);
}
