#include "npc.hpp"
#include "construct.hpp"

GeneratedObject genmesh_generate_npc()
{
    GeneratedObject npc = {};

    // Legs (Grey)
    npc.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.3, 0.6, 0.3}), {-0.2, 0, 0}),
        Color::hex(0x424242FF), true);
    npc.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.3, 0.6, 0.3}), {0.2, 0, 0}),
        Color::hex(0x424242FF), true);
        
    // Torso (Orange)
    npc.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.8, 0.8, 0.4}), {0, 0.6, 0}),
        Color::hex(0xff9800FF), true);
        
    // Arms (Skin tone)
    npc.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.25, 0.7, 0.25}), {-0.525, 0.7, 0}),
        Color::hex(0xffccaaFF), true);
    npc.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.25, 0.7, 0.25}), {0.525, 0.7, 0}),
        Color::hex(0xffccaaFF), true);

    // Head (Skin tone)
    npc.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.6, 0.6, 0.6}), {0, 1.4, 0}),
        Color::hex(0xffccaaFF), true);

    return npc;
}
