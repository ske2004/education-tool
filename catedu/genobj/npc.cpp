#include "npc.hpp"
#include "construct.hpp"

GeneratedObject genmesh_generate_npc()
{
    GeneratedObject npc = {};

    // Body
    Box3 body = construct_box(Baseline::Bottom, {0.8, 1.2, 0.6});
    npc.push_colored_box(body, Color::hex(0x3355AAFF), true);

    // Head
    Box3 head = construct_box(Baseline::Bottom, {0.5, 0.5, 0.5});
    head = box3_translate(head, {0, 1.2, 0});
    npc.push_colored_box(head, Color::hex(0xFFCC88FF), true);

    return npc;
}
