#include "bridge.hpp"

GeneratedObject genmesh_generate_bridge()
{
    GeneratedObject obj = {};
    // Wooden plank floor
    obj.push_colored_box({{-0.5f, -0.05f, -0.5f}, {0.5f, 0.05f, 0.5f}}, Color::hex(0x8B4513FF));
    // Small side railings
    obj.push_colored_box({{-0.5f, 0.05f, -0.45f}, {0.5f, 0.25f, -0.35f}}, Color::hex(0x5C3A21FF));
    obj.push_colored_box({{-0.5f, 0.05f, 0.35f}, {0.5f, 0.25f, 0.45f}}, Color::hex(0x5C3A21FF));
    return obj;
}
