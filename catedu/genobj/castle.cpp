#include "castle.hpp"
#include "construct.hpp"
#include <cassert>

#define FLOOR_HEIGHT 2
#define BUILDING_SIZE_X 8
#define BUILDING_SIZE_Z 8
#define CASTLE_COLOR_STONE 0x888888FF
#define CASTLE_COLOR_DARK 0x555555FF
#define CASTLE_COLOR_DOOR 0x4E342EFF

GeneratedObject genmesh_generate_castle(int floor_count)
{
    GeneratedObject object = {};
    assert(floor_count > 0);

    // Base body
    Vector3 size = {BUILDING_SIZE_X, (float)(FLOOR_HEIGHT * floor_count), BUILDING_SIZE_Z};
    object.push_colored_box(
        construct_box(Baseline::Bottom, size),
        Color::hex(CASTLE_COLOR_STONE), true);

    // Door
    object.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {2, 2.5f, 0.2f}),
                       {0, 0, -4}),
        Color::hex(CASTLE_COLOR_DOOR), true);

    // Roof battlements (crenellations)
    float roof_y = (float)(FLOOR_HEIGHT * floor_count);
    
    // Corner towers
    for (int tx = -1; tx <= 1; tx += 2) {
        for (int tz = -1; tz <= 1; tz += 2) {
            Vector3 tpos = {(float)(tx * 3.5), roof_y, (float)(tz * 3.5)};
            object.push_colored_box(
                box3_translate(construct_box(Baseline::Bottom, {1.5f, 1.2f, 1.5f}), tpos),
                Color::hex(CASTLE_COLOR_DARK), true);
        }
    }
    
    // Side battlements
    for (int i = -2; i <= 2; i += 2) {
        // Front & Back
        object.push_colored_box(box3_translate(construct_box(Baseline::Bottom, {0.8f, 0.8f, 0.4f}), {(float)i, roof_y, -3.8f}), Color::hex(CASTLE_COLOR_STONE), true);
        object.push_colored_box(box3_translate(construct_box(Baseline::Bottom, {0.8f, 0.8f, 0.4f}), {(float)i, roof_y, 3.8f}), Color::hex(CASTLE_COLOR_STONE), true);
        // Left & Right
        object.push_colored_box(box3_translate(construct_box(Baseline::Bottom, {0.4f, 0.8f, 0.8f}), {-3.8f, roof_y, (float)i}), Color::hex(CASTLE_COLOR_STONE), true);
        object.push_colored_box(box3_translate(construct_box(Baseline::Bottom, {0.4f, 0.8f, 0.8f}), {3.8f, roof_y, (float)i}), Color::hex(CASTLE_COLOR_STONE), true);
    }

    return object;
}
