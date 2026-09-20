#include "high_grass.hpp"
#include "construct.hpp"

GeneratedObject genmesh_generate_high_grass()
{
    GeneratedObject obj = {};
    obj.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.8f, 0.4f, 0.8f}), {0, 0, 0}),
        Color::hex(0x4CAF50FF), true); // Green grass
    
    // Add some random blades
    obj.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.1f, 0.6f, 0.1f}), {-0.2f, 0.4f, -0.2f}),
        Color::hex(0x388E3CFF), true);
    obj.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.1f, 0.7f, 0.1f}), {0.3f, 0.4f, 0.1f}),
        Color::hex(0x388E3CFF), true);

    return obj;
}
