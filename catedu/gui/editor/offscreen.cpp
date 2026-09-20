#include "offscreen.hpp"
#include "catedu/core/storage/stack.hpp"
#include <cstdio>

Stack<OffscreenTarget> cache;
size_t available_target = 0;

static OffscreenTarget create(UiRenderingCore *core, Vector2i size)
{
    OffscreenTarget target = {0};

    sg_image_desc img_desc = {};
    img_desc.render_target = true;
    img_desc.width = size.x * 2;
    img_desc.height = size.y * 2;
    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;

    sg_sampler_desc sampler_desc = {};
    sampler_desc.min_filter = SG_FILTER_LINEAR;
    sampler_desc.mag_filter = SG_FILTER_LINEAR;
    sampler_desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
    sampler_desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;

    target.img = sg_make_image(&img_desc);
    img_desc.pixel_format = SG_PIXELFORMAT_DEPTH;
    target.depth = sg_make_image(&img_desc);
    target.sampler = sg_make_sampler(&sampler_desc);

    sg_attachments_desc att_desc = {};
    att_desc.colors[0].image = target.img;
    att_desc.depth_stencil.image = target.depth;
    target.attachment = sg_make_attachments(att_desc);

    UiImage img = {};
    img.fliph = !sg_query_features().origin_top_left;
    img.sampler = target.sampler;
    img.image = target.img;
    img.size = {size.x, size.y};

    target.id = core->alloc_image(img);

    return target;
}

void offscreen_init_targets()
{
    printf("INIT: offscreen targets\n");
}

sg_pass_action offscreen_pass_action()
{
    sg_pass_action result = {};
    result.colors[0].load_action = SG_LOADACTION_CLEAR;
    result.colors[0].store_action = SG_STOREACTION_STORE;
    result.colors[0].clear_value = {0.0f, 0.0f, 0.0f, 0.0f};
    return result;
}

void offscreen_clear()
{
    available_target = 0;
}

sg_attachments offscreen_alloc(UiRenderingCore *core, UiImageId &id_out)
{
    if (available_target >= cache.count)
    {
        printf("OFFSCREEN: Expanding cache size to %zu\n", cache.count);
        cache.push(create(core, {128, 128}));
        id_out = cache[available_target].id;
        return cache[available_target++].attachment;
    }

    id_out = cache[available_target].id;
    return cache[available_target++].attachment;
}

void offscreen_deinit_targets(UiRenderingCore *core)
{
    printf("DEINIT: offscreen targets\n");

    for (size_t i = 0; i < cache.count; i++)
    {
        sg_destroy_image(cache[i].depth);
        sg_destroy_attachments(cache[i].attachment);

        // The image and sampler is destroyed by the core
        core->dealloc_image(cache[i].id);
    }

    cache.deinit();
    cache = {};
    available_target = 0;
}
