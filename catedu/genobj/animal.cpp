#include "animal.hpp"
#include "construct.hpp"

GeneratedObject genmesh_generate_animal()
{
    GeneratedObject animal = {};

    // Legs (White/Grey)
    animal.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.2, 0.4, 0.2}), {-0.2, 0, -0.3}),
        Color::hex(0xEEEEEEFF), true);
    animal.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.2, 0.4, 0.2}), {0.2, 0, -0.3}),
        Color::hex(0xEEEEEEFF), true);
    animal.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.2, 0.4, 0.2}), {-0.2, 0, 0.3}),
        Color::hex(0xEEEEEEFF), true);
    animal.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.2, 0.4, 0.2}), {0.2, 0, 0.3}),
        Color::hex(0xEEEEEEFF), true);
        
    // Torso/Body (White Sheep/Cow/Dog)
    animal.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.7, 0.6, 1.0}), {0, 0.4, 0}),
        Color::hex(0xFFFFFFFF), true);
        
    // Head (Pinkish nose or dark)
    animal.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.4, 0.4, 0.4}), {0, 0.6, 0.6}),
        Color::hex(0xDDDDDDFF), true);

    return animal;
}
