#include "prop.hpp"
#include <string.h>

GeneratedObject genmesh_generate_prop(const char* id)
{
    GeneratedObject obj = {};
    
    if (id && strcmp(id, "bench") == 0) {
        // Seat
        obj.push_colored_box({{-0.4f, 0.2f, -0.2f}, {0.4f, 0.3f, 0.2f}}, Color::hex(0x8B4513FF));
        // Back
        obj.push_colored_box({{-0.4f, 0.3f, -0.2f}, {0.4f, 0.6f, -0.1f}}, Color::hex(0x8B4513FF));
        // Legs
        obj.push_colored_box({{-0.35f, 0.0f, -0.15f}, {-0.25f, 0.2f, -0.05f}}, Color::hex(0x555555FF));
        obj.push_colored_box({{0.25f, 0.0f, -0.15f}, {0.35f, 0.2f, -0.05f}}, Color::hex(0x555555FF));
        obj.push_colored_box({{-0.35f, 0.0f, 0.05f}, {-0.25f, 0.2f, 0.15f}}, Color::hex(0x555555FF));
        obj.push_colored_box({{0.25f, 0.0f, 0.05f}, {0.35f, 0.2f, 0.15f}}, Color::hex(0x555555FF));
    } else if (id && strcmp(id, "trash_can") == 0) {
        obj.push_colored_box({{-0.2f, 0.0f, -0.2f}, {0.2f, 0.5f, 0.2f}}, Color::hex(0x444444FF));
        obj.push_colored_box({{-0.22f, 0.5f, -0.22f}, {0.22f, 0.55f, 0.22f}}, Color::hex(0x222222FF));
    } else if (id && strcmp(id, "street_light") == 0) {
        // Pole
        obj.push_colored_box({{-0.05f, 0.0f, -0.05f}, {0.05f, 1.5f, 0.05f}}, Color::hex(0x333333FF));
        // Arm
        obj.push_colored_box({{-0.05f, 1.4f, -0.05f}, {0.4f, 1.5f, 0.05f}}, Color::hex(0x333333FF));
        // Light
        obj.push_colored_box({{0.3f, 1.3f, -0.1f}, {0.5f, 1.4f, 0.1f}}, Color::hex(0xFFFFDDFF));
    } else if (id && strcmp(id, "flower_pot") == 0) {
        // Pot
        obj.push_colored_box({{-0.25f, 0.0f, -0.25f}, {0.25f, 0.4f, 0.25f}}, Color::hex(0xCD853FFF));
        // Plant/Flower
        obj.push_colored_box({{-0.15f, 0.4f, -0.15f}, {0.15f, 0.8f, 0.15f}}, Color::hex(0x228B22FF));
        obj.push_colored_box({{-0.2f, 0.7f, -0.2f}, {0.2f, 0.9f, 0.2f}}, Color::hex(0xFF1493FF));
    } else if (id && strcmp(id, "fountain") == 0) {
        // Base
        obj.push_colored_box({{-0.8f, 0.0f, -0.8f}, {0.8f, 0.2f, 0.8f}}, Color::hex(0x888888FF));
        // Pillar
        obj.push_colored_box({{-0.2f, 0.2f, -0.2f}, {0.2f, 1.0f, 0.2f}}, Color::hex(0xAAAAAAFF));
        // Water top
        obj.push_colored_box({{-0.1f, 1.0f, -0.1f}, {0.1f, 1.2f, 0.1f}}, Color::hex(0x4488FFFF));
        // Water pool
        obj.push_colored_box({{-0.7f, 0.1f, -0.7f}, {0.7f, 0.25f, 0.7f}}, Color::hex(0x4488FFFF));
    } else {
        // Default generic prop box
        obj.push_colored_box({{-0.4f, 0.0f, -0.4f}, {0.4f, 0.8f, 0.4f}}, Color::hex(0x888888FF));
    }
    
    return obj;
}
