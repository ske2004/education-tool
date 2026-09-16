#include "sg_tricks.hpp"

static sg_image t_white_image;
static sg_sampler t_white_sampler;
static bool is_init;

void sg_tricks_init()
{
    if (is_init) return;
    is_init = true;

    // init white pixel
    uint8_t dat[4] = {0xff, 0xff, 0xff, 0xff};
    sg_image_desc image_desc = {};
    image_desc.width = 1;
    image_desc.height = 1;
    image_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    image_desc.label = "educat-white-pixel-tex";
    image_desc.data.subimage[0][0].ptr = dat;
    image_desc.data.subimage[0][0].size = 4;
    sg_sampler_desc sampler_desc = {};
    sampler_desc.min_filter = SG_FILTER_LINEAR;
    sampler_desc.mag_filter = SG_FILTER_LINEAR;
    sampler_desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
    sampler_desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
    sampler_desc.label = "educat-white-pixel-sam";

    t_white_image = sg_make_image(image_desc);
    t_white_sampler = sg_make_sampler(sampler_desc);
}

void sg_tricks_get_white_texture(sg_image &image, sg_sampler &sampler)
{
    sg_tricks_init();
    image = t_white_image;
    sampler = t_white_sampler;
}

void sg_tricks_deinit()
{
    sg_destroy_sampler(t_white_sampler);
    sg_destroy_image(t_white_image);
}
