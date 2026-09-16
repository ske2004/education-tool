#include "font.hpp"
#include "make_brush.hpp"
#include "pass.hpp"
#include "transform.hpp"
#include <cstdio>

#define PAGE_IMAGE_SIZE 1024

bool valid_glyph(int glyph)
{
    return glyph >= 0 && glyph < (256 * 256);
}

void populate_chunk(UiFontRenderer *renderer, int i)
{
    if (i < 0 || i > 255)
    {
        return;
    }

    if (renderer->chunks[i])
    {
        return;
    }

    UiFontChunk *chunk =
        (UiFontChunk *)ALLOCATOR_MALLOC.alloc(sizeof(UiFontChunk));
    *chunk = {0};

    stbtt_pack_context pack_context = {};

    std::vector<unsigned char> pixels(PAGE_IMAGE_SIZE * PAGE_IMAGE_SIZE);
    std::vector<unsigned char> pixels_rgba(PAGE_IMAGE_SIZE * PAGE_IMAGE_SIZE *
                                           4);
    stbtt_PackBegin(&pack_context, pixels.data(), PAGE_IMAGE_SIZE,
                    PAGE_IMAGE_SIZE, PAGE_IMAGE_SIZE, 2, nullptr);

    stbtt_PackFontRange(&pack_context, renderer->buf.data, 0,
                        renderer->def.font_size * renderer->scale_factor,
                        i * 256, 256, chunk->packed_chars);
    stbtt_PackEnd(&pack_context);

    for (int i = 0; i < pixels.size(); i += 1)
    {
        int alpha = pixels[i];
        pixels_rgba[i * 4 + 0] = 0xFF;
        pixels_rgba[i * 4 + 1] = 0xFF;
        pixels_rgba[i * 4 + 2] = 0xFF;
        pixels_rgba[i * 4 + 3] = alpha;
    }

    chunk->image_size = PAGE_IMAGE_SIZE;

    chunk->image = renderer->core->alloc_image(
        ui_image_make_from_data({pixels_rgba.data(), pixels_rgba.size()},
                                {PAGE_IMAGE_SIZE, PAGE_IMAGE_SIZE}));

    renderer->chunks[i] = chunk;
}

stbtt_packedchar *get_packed_char(UiFontRenderer *f, int chara)
{
    if (!valid_glyph(chara))
    {
        return nullptr;
    }

    populate_chunk(f, chara / 256);
    if (!f->chunks[chara / 256])
    {
        return nullptr;
    }
    return &f->chunks[chara / 256]->packed_chars[chara % 256];
}

static Vector2 get_glyph_size(UiFontRenderer *f, int glyph)
{
    stbtt_packedchar *pc = get_packed_char(f, glyph);

    if (pc)
    {
        return {float(pc->x1 - pc->x0), float(pc->y1 - pc->y0)};
    }

    return {0, 0};
}

UiFontRenderer UiFontRenderer::init(UiRenderingCore *core, UiFontDef def,
                                    float scale)
{
    UiFontRenderer result = {};
    result.core = core;
    result.def = def;
    result.scale_factor = scale;

    FILE *f = fopen(def.path, "rb");
    if (!f)
    {
        fprintf(stderr, "UiFontRenderer::init: failed to open font '%s'\n", def.path);
        return result;
    }
    FileBuffer buf = FileBuffer::read_whole_file(f);
    fclose(f);
    result.buf = buf;

    stbtt_InitFont(&result.font_info, buf.data, 0);
    result.scale = stbtt_ScaleForPixelHeight(
        &result.font_info, def.font_size * result.scale_factor);
    int ascent, descent;
    stbtt_GetFontVMetrics(&result.font_info, &ascent, &descent, nullptr);
    result.ascent = ascent;
    result.descent = descent;

    populate_chunk(&result, 0);
    return result;
}

void UiFontRenderer::render_glyph(UiRenderingPass *pass, Vector2 position,
                                  int glyph, UiBrush brush, Vector2 scale)
{
    auto *packed_char = get_packed_char(this, glyph);
    if (!packed_char)
    {
        return;
    }

    Rect image_region = {{float(packed_char->x1), float(packed_char->y0)},
                         {float(packed_char->x0) - float(packed_char->x1),
                          float(packed_char->y1) - float(packed_char->y0)}};

    auto new_brush = UiMakeBrush::make_image_brush(
                         pass->core, this->chunks[glyph / 256]->image)
                         .with_image_region(image_region)
                         .with_gradient(brush.color_bottom, brush.color_top)
                         .build();

    UiTransform transform = {};
    transform.base = {position, get_glyph_size(this, glyph)};
    transform.scale = scale;

    pass->push_transform(transform);
    pass->core->render_object(new_brush);
    pass->pop_transform();
}

static inline size_t utf8_fetch(uint32_t *out, const char *s_)
{
    const unsigned char *s = (const unsigned char *)s_;
    if ((*s & 0xC0) != 0xC0)
    {
        *out = *s;
        return *s > 0;
    }

    const static size_t clas[32] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                    1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
                                    0, 0, 2, 2, 2, 2, 3, 3, 4, 5};
    size_t cl = clas[*s >> 3];

    for (size_t i = 1; i < cl; ++i)
    {
        if ((s[i] & 0xC0) == 0xC0 || (s[i] & 0x80) == 0)
        {
            *out = s[0];
            return 1;
        }
    }

    switch (cl)
    {
    case 2:
        *out = ((s[0] & 0x1f) << 6) | (s[1] & 0x3f);
        break;
    case 3:
        *out = ((s[0] & 0xf) << 12) | ((s[1] & 0x3f) << 6) | (s[2] & 0x3f);
        break;
    case 4:
        *out = ((s[0] & 0x7) << 18) | ((s[1] & 0x3f) << 12) |
               ((s[2] & 0x3f) << 6) | (s[3] & 0x3f);
        break;
    default:
        *out = 0;
        return 0;
    }

    return cl;
}

void UiFontRenderer::render_text_utf8(UiRenderingPass *pass, Vector2 position,
                                      const char *text, UiBrush brush,
                                      Vector2 scale)
{
    scale /= this->scale_factor;
    Vector2 pos = {0, 0};

    for (size_t i = 0; text[i];)
    {
        uint32_t chara = 0;
        i += utf8_fetch(&chara, text + i);
        populate_chunk(this, chara / 256);
        UiFontChunk *chk = this->chunks[chara / 256];

        if (chara == '\n')
        {
            pos.x = 0;
            pos.y += ((this->ascent - this->descent) * this->scale);

            continue;
        }

        stbtt_aligned_quad quad;
        stbtt_GetPackedQuad(chk->packed_chars, chk->image_size, chk->image_size,
                            chara % 256, &pos.x, &pos.y, &quad, false);

        render_glyph(
            pass,
            position +
                Vector2{quad.x0, quad.y0 + this->ascent * this->scale} * scale,
            chara, brush, scale);
    }
}

Rect UiFontRenderer::bounds_text_utf8(Vector2 position, const char *text,
                                      Vector2 scale, size_t until)
{
    scale /= this->scale_factor;
    Vector2 pos = {0, 0};
    float max_w = 0;
    int chara_num = 0;

    for (size_t i = 0; text[i];)
    {
        if (chara_num == until)
        {
            break;
        }

        uint32_t chara = 0;
        i += utf8_fetch(&chara, text + i);
        chara_num++;

        populate_chunk(this, chara / 256);
        UiFontChunk *chk = this->chunks[chara / 256];

        if (chara == '\n')
        {
            if (pos.x > max_w)
            {
                max_w = pos.x;
            }
            pos.x = 0;
            pos.y += ((this->ascent - this->descent) * this->scale) * scale.y;

            continue;
        }

        stbtt_aligned_quad quad;
        stbtt_GetPackedQuad(chk->packed_chars, chk->image_size, chk->image_size,
                            chara % 256, &pos.x, &pos.y, &quad, false);
    }
    pos.y += ((this->ascent - this->descent) * this->scale) * scale.y;

    if (pos.x > max_w)
    {
        max_w = pos.x;
    }
    pos.x = max_w * scale.x;

    return {position, pos};
}

void UiFontRenderer::deinit()
{
    this->buf.deinit();
    for (int i = 0; i < 256; i++)
    {
        if (this->chunks[i])
        {
            this->core->dealloc_image(this->chunks[i]->image);
            ALLOCATOR_MALLOC.free(this->chunks[i]);
        }
    }
}
