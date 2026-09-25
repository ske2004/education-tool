#include "prop.hpp"
#include <string.h>

const PropModel PROP_MODELS[] = {
    {"barrel", 0.5f},  {"car", 0.37f},      {"counter", 0.5f},
    {"crate", 0.5f},   {"tile", 0.7f},      {"pavement", 0.7f},
    {"woodtile", 0.7f}, {"wall", 0.5f},     {"wall_wood", 0.5f},
    {"tree", 0.12f},   {"player", 0.5f},    {"npc", 0.5f},
};
const size_t PROP_MODEL_COUNT = sizeof(PROP_MODELS) / sizeof(PROP_MODELS[0]);

static const char *const GENERATED_PROP_IDS[] = {
    "bench", "trash_can", "street_light", "flower_pot", "fountain",
};

const char *const PROP_IDS[] = {
    "bench",   "trash_can", "street_light", "flower_pot", "fountain",
    "barrel",  "car",       "counter",      "crate",      "tile",
    "pavement", "woodtile", "wall",         "wall_wood",  "tree",
    "player",  "npc",
};
const size_t PROP_ID_COUNT = sizeof(PROP_IDS) / sizeof(PROP_IDS[0]);

const PropModel *find_prop_model(const char *id)
{
    for (size_t i = 0; id && i < PROP_MODEL_COUNT; i++)
    {
        if (strcmp(id, PROP_MODELS[i].id) == 0)
        {
            return &PROP_MODELS[i];
        }
    }

    return nullptr;
}

bool genmesh_is_generated_prop(const char *id)
{
    for (const char *generated : GENERATED_PROP_IDS)
    {
        if (id && strcmp(id, generated) == 0)
        {
            return true;
        }
    }

    return false;
}

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
