#include "player.hpp"
#include "construct.hpp"

GeneratedObject genmesh_generate_player()
{
    GeneratedObject player = {};

    // Legs (Dark Blue)
    player.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.3, 0.6, 0.3}), {-0.2, 0, 0}),
        Color::hex(0x1a237eFF), true);
    player.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.3, 0.6, 0.3}), {0.2, 0, 0}),
        Color::hex(0x1a237eFF), true);
        
    // Torso (Cyan)
    player.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.8, 0.8, 0.4}), {0, 0.6, 0}),
        Color::hex(0x00bcd4FF), true);
        
    // Arms (Skin tone/Cyan)
    player.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.25, 0.7, 0.25}), {-0.525, 0.7, 0}),
        Color::hex(0xffccaaFF), true);
    player.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.25, 0.7, 0.25}), {0.525, 0.7, 0}),
        Color::hex(0xffccaaFF), true);

    // Head (Skin tone)
    player.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.6, 0.6, 0.6}), {0, 1.4, 0}),
        Color::hex(0xffccaaFF), true);

    return player;
}
