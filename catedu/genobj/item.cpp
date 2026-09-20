#include "item.hpp"
#include "catedu/genobj/construct.hpp"
#include "catedu/genobj/generator.hpp"

GeneratedObject genmesh_generate_item()
{
    GeneratedObject obj = {};
    // Small coin/bag (Floating above the ground/road)
    obj.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.5f, 0.3f, 0.5f}), {0.0f, 0.3f, 0.0f}),
        Color::hex(0x8B4513FF), true); // Brown base
    obj.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.4f, 0.2f, 0.4f}), {0.0f, 0.6f, 0.0f}),
        Color::hex(0xFFD700FF), true); // Gold top
    return obj;
}
