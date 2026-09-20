#include "water.hpp"
#include "construct.hpp"

GeneratedObject genmesh_generate_water()
{
    GeneratedObject obj = {};
    obj.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {1.0f, 0.05f, 1.0f}), {0, 0.02f, 0}),
        Color::hex(0x2196F3FF), true); // Blue water
    return obj;
}
