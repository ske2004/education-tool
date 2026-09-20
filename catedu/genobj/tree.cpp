#include "tree.hpp"
#include "catedu/genobj/construct.hpp"
#include "catedu/genobj/generator.hpp"

GeneratedObject genmesh_generate_tree()
{
    GeneratedObject obj = {};
    // Trunk (Brown)
    obj.push_colored_box(construct_box(Baseline::Bottom, {0.3, 1.5, 0.3}),
                         Color::hex(0x5C4033FF), true);
    
    // Bottom leaves layer
    obj.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {1.5, 1.2, 1.5}), {0, 1.0, 0}),
        Color::hex(0x2E8B57FF), true);
        
    // Middle leaves layer
    obj.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {1.1, 1.0, 1.1}), {0, 2.0, 0}),
        Color::hex(0x3CB371FF), true);
        
    // Top leaves layer
    obj.push_colored_box(
        box3_translate(construct_box(Baseline::Bottom, {0.7, 0.8, 0.7}), {0, 2.8, 0}),
        Color::hex(0x90EE90FF), true);
        
    return obj;
}
