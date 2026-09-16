#include "texture.hpp"
#include "catedu/core/memory/buffer.hpp"
#include "lib/stb/stb_image.h"
#include <assert.h>
#include <stdio.h>

#ifdef _WIN32
#include <io.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif

void init_sampler_tex(sg_sampler &s, sg_image &i, Vector2 size, Buffer data)
{
    sg_image_desc idesc = {};
    idesc.width = size.x;
    idesc.height = size.y;
    idesc.pixel_format = SG_PIXELFORMAT_RGBA8;
    idesc.label = "educat-generic-image";
    idesc.data.subimage[0][0].ptr = data.data;
    idesc.data.subimage[0][0].size = data.size;
    sg_sampler_desc sdesc = {};
    sdesc.min_filter = SG_FILTER_LINEAR;
    sdesc.mag_filter = SG_FILTER_LINEAR;
    sdesc.wrap_u = SG_WRAP_REPEAT;
    sdesc.wrap_v = SG_WRAP_REPEAT;
    sdesc.label = "educat-generic-sampler";

    i = sg_make_image(idesc);
    s = sg_make_sampler(sdesc);
}

Texture Texture::init(const char *png_path)
{
    assert(png_path && "Texture::init: png_path is null");
    if (access(png_path, F_OK) != 0)
    {
        fprintf(stderr, "Texture::init: file %s does not exist\n", png_path);
    }
    Texture result = {};
    int width, height, channels;
    stbi_uc *data = stbi_load(png_path, &width, &height, &channels, 4);
    if (!data) {
        fprintf(stderr, "Texture::init: stbi_load failed for '%s'\n", png_path);
        return result;
    }
    Buffer data_buf = {data, size_t(width * height * 4)};
    Vector2 data_dims = {(float)width, (float)height};
    result.size = data_dims;

    init_sampler_tex(result.sysid_sampler, result.sysid_texture, data_dims,
                     data_buf);

    stbi_image_free(data);

    return result;
}

void Texture::deinit()
{
    sg_destroy_sampler(this->sysid_sampler);
    sg_destroy_image(this->sysid_texture);
}

Texture Texture::uncropped()
{
    Texture tex = *this;
    tex.crop = {};
    tex.if_crop = false;
    return tex;
}

Texture Texture::cropped(Rect crop)
{
    Texture tex = *this;
    tex.crop = crop;
    tex.if_crop = true;
    return tex;
}

Vector2 Texture::uv_min()
{
    return {crop.pos.x / size.x, crop.pos.y / size.y};
}

Vector2 Texture::uv_max()
{
    return {(crop.pos.x + crop.siz.x) / size.x,
            (crop.pos.y + crop.siz.y) / size.y};
}

bool Texture::if_valid()
{
    return this->sysid_texture.id != SG_INVALID_ID &&
           this->sysid_sampler.id != SG_INVALID_ID;
}
