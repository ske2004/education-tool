#include "widgets.hpp"
#include "catedu/core/math/math.hpp"
#include "catedu/sys/input.hpp"
#include "catedu/ui/rendering/core.hpp"
#include "catedu/ui/rendering/make_brush.hpp"
#include <cstring>

static const Vector4 theme[] = {
    {0.9, 0.9, 1.0, 1.0}, {0.7, 0.7, 0.8, 1.0}, // Button
    {0.1, 0.1, 0.2, 1.0},                       // Text
    {0.1, 0.1, 0.2, 1.0},                       // Border
    {0.7, 0.7, 0.9, 1.0}, {0.6, 0.6, 0.7, 1.0}, // Button Pressed
    {1.0, 1.0, 1.0, 1.0}                        // Window
};

bool begin_show_window(UiPass &user, WindowInfo info)
{
    // TODO: Create a formatter that will use the UI state
    char buffer[256];
    snprintf(buffer, 256, "%s#window", info.title);
    user.state->element_storage.push(buffer, {false, {}, {}, info.rect});

    UiPersistentElement *pe = user.state->element_storage.value();
    pe->scale = slerp(pe->scale, 1.0f, 25.0f, sapp_frame_duration());

    if (user.hovered() && user.state->input.k[INPUT_MB_LEFT].pressed)
    {
        pe->pin = user.state->input.mouse_pos - pe->persistent_box.pos;
        pe->pinned = true;
    }
    if (user.hovered() && user.state->input.k[INPUT_MB_RIGHT].pressed)
    {
        pe->hidden = !pe->hidden;
    }
    if (pe->pinned && user.state->input.k[INPUT_MB_LEFT].held)
    {
        pe->persistent_box.pos +=
            ((user.state->input.mouse_pos - pe->persistent_box.pos) - pe->pin) /
            user.state->dpi_scale;
        pe->pin = user.state->input.mouse_pos - pe->persistent_box.pos;
    }
    else
    {
        pe->pinned = false;
    }

    pe->persistent_box.siz.x = std::max(pe->persistent_box.siz.x, 100.0f);
    pe->persistent_box.siz.y = std::max(pe->persistent_box.siz.y, 100.0f);

    if (!info.if_static)
    {
        info.rect = pe->persistent_box;
    }

    UiMakeBrush base = UiMakeBrush::make_plain_brush().squircle(0.75);

    AutoLayoutElement cel = {};
    cel.position = AutoLayoutPosition::absolute;
    cel.offset = info.rect.pos;
    cel.width.type = AutoLayoutDimension::autom;
    cel.height.type = AutoLayoutDimension::autom;
    cel.layout.type = AutoLayout::column;
    cel.border = {1, 1, 2, 1};
    user.begin_generic(cel, {},
                       base.with_gradient(0x00000044, 0x00000000).build(),
                       user.state->element_storage.id(), pe->scale);

    // Titlebar
    {
        AutoLayoutElement el = {};
        el.border = {3, 3, 3, 3};
        el.width.type = AutoLayoutDimension::pixel;
        el.width.value = info.rect.siz.x - 6;
        el.height.type = AutoLayoutDimension::autom;
        el.padding = {6, 6, 6, 6};
        el.clip = true;

        UiBrush border = base.with_gradient(theme[0] * Vector4{1, 1, 1, 0.9},
                                            theme[1] * Vector4{1, 1, 1, 0.7})
                             .squircle(0.6, 0.6, 1, 1)
                             .build();
        user.begin_generic(el, {}, border);
        label(user, info.title, {1.5, 1.5});
        user.end_generic();
    }

    // Content
    {
        AutoLayoutElement el = {};
        el.border = {3, 3, 3, 3};
        el.padding = {3, 3, 3, 3};
        el.width.type = AutoLayoutDimension::pixel;
        el.width.value = info.rect.siz.x;
        el.height.type = AutoLayoutDimension::pixel;
        el.height.value = info.rect.siz.y;
        el.hidden = pe->hidden;
        el.clip = true;

        UiBrush border =
            base.with_solid(theme[0]).squircle(1, 1, 0.75, 0.75).build();
        UiBrush background = base.with_solid(theme[6]).build();
        user.begin_generic(el, background, border);
    }

    // Resize handle
    {
        AutoLayoutElement el = {};
        el.width.type = AutoLayoutDimension::pixel;
        el.width.value = 6;
        el.height.type = AutoLayoutDimension::pixel;
        el.height.value = 6;
        el.clip = true;
        el.position = AutoLayoutPosition::detached;
        el.offset = info.rect.siz;

        user.state->element_storage.push(".resize", {true, {}, {}, {}});
        user.begin_generic(el, {}, {}, user.state->element_storage.id());

        if (user.active() && user.state->input.k[INPUT_MB_LEFT].held)
        {
            pe->persistent_box.siz +=
                user.state->input.mouse_delta / user.state->dpi_scale;
        }

        user.end_generic();
        user.state->element_storage.pop();
    }

    // Resize handle bottom
    {
        AutoLayoutElement el = {};
        el.width.type = AutoLayoutDimension::pixel;
        el.width.value = info.rect.siz.x;
        el.height.type = AutoLayoutDimension::pixel;
        el.height.value = 6;
        el.clip = true;
        el.position = AutoLayoutPosition::detached;
        el.offset = {0, info.rect.siz.y};

        user.state->element_storage.push(".resizev", {true, {}, {}, {}});
        user.begin_generic(el, {}, {}, user.state->element_storage.id());

        if (user.active() && user.state->input.k[INPUT_MB_LEFT].held)
        {
            pe->persistent_box.siz.y +=
                user.state->input.mouse_delta.y / user.state->dpi_scale;
        }

        user.end_generic();
        user.state->element_storage.pop();
    }

    // Resize handle right
    {
        AutoLayoutElement el = {};
        el.width.type = AutoLayoutDimension::pixel;
        el.width.value = 6;
        el.height.type = AutoLayoutDimension::pixel;
        el.height.value = info.rect.siz.y;
        el.clip = true;
        el.position = AutoLayoutPosition::detached;
        el.offset = {info.rect.siz.x, 0};

        user.state->element_storage.push(".resizeh", {true, {}, {}, {}});
        user.begin_generic(el, {}, {}, user.state->element_storage.id());

        if (user.active() && user.state->input.k[INPUT_MB_LEFT].held)
        {
            pe->persistent_box.siz.x +=
                user.state->input.mouse_delta.x / user.state->dpi_scale;
        }

        user.end_generic();
        user.state->element_storage.pop();
    }

    return !pe->hidden;
}

void end_show_window(UiPass &user)
{
    user.end_generic();
    user.end_generic();

    user.state->element_storage.pop();
}

void begin_button_frame(UiPass &user, const char *id, AutoLayoutElement el,
                        Vector4 background, float radius)
{
    user.state->element_storage.push(id, {});

    el.position = AutoLayoutPosition::relative;

    Vector4 color_top = theme[1] * background;
    Vector4 color_bottom = theme[0] * background;
    auto v = user.state->element_storage.value();
    if (v->scale < 1)
    {
        v->scale = 1;
    }

    color_top.w = 0.4f;

    if (user.actively_hovered())
    {
        color_top = theme[5] * background;
        color_bottom = theme[4] * background;
        v->scale = slerp(v->scale, 1.125, 20, sapp_frame_duration());
        color_top.w *= v->scale;
        color_bottom.w *= v->scale;
        if (color_top.w > 1)
        {
            color_top.w = 1;
        }
        if (color_bottom.w > 1)
        {
            color_bottom.w = 1;
        }
    }
    else
    {
        v->scale = slerp(v->scale, 1, 20, sapp_frame_duration());
    }

    if (user.state->input.k[INPUT_MB_LEFT].held && user.active())
    {
        std::swap(color_top, color_bottom);
    }

    UiBrush base = UiMakeBrush::make_plain_brush()
                       .squircle(radius)
                       .with_gradient(color_bottom, color_top)
                       .build();
    UiBrush border = UiMakeBrush::make_plain_brush()
                         .squircle(radius)
                         .with_gradient(0x00000088, 0x00000000)
                         .build();

    user.begin_generic(el, base, border, user.state->element_storage.id(),
                       v->scale);
}

bool end_button_frame(UiPass &user)
{
    bool pressed =
        user.actively_hovered() && user.active() &&
        user.state->input.k[INPUT_MB_LEFT].released;
    user.end_generic();
    user.state->element_storage.pop();
    return pressed;
}

bool button(UiPass &user, const char *text, Vector4 background)
{
    AutoLayoutElement el = {};
    el.padding = {3, 3, 3, 3};
    el.margin = {1, 1, 1, 1};
    el.border = {1, 1, 1, 1};

    begin_button_frame(user, text, el, background, 0.5);
    label(user, text, {1.5, 1.5}, UiMakeBrush::make_solid(theme[3]));
    return end_button_frame(user);
}

bool button_toggle(UiPass &user, const char *text, bool &state)
{
    if (button(user, text,
               state ? Vector4{0.8, 1.0, 0.8, 1.0}
                     : Vector4{1.0, 1.0, 1.0, 1.0}))
    {
        state = !state;
    }
    return state;
}

bool button_radio(UiPass &user, const char *text, int &mode, int val)
{
    const Vector4 color =
        mode == val ? Vector4{0.8, 1.0, 0.8, 1.0} : Vector4{1.0, 1.0, 1.0, 1.0};
    if (button(user, text, color))
    {
        mode = val;
        return true;
    }
    return false;
}

void img(UiPass &user, const char *path, Vector2 scale)
{
    UiImageId id = user.state->get_image(path);

    AutoLayoutElement el = {};
    el.layout = {AutoLayout::row};
    Vector2 size = vector2i_to_vector2(user.state->core->get_image(id)->size);

    el.width = {AutoLayoutDimension::pixel, size.x * scale.x};
    el.height = {AutoLayoutDimension::pixel, size.y * scale.y};

    user.begin_generic(
        el, UiMakeBrush::make_image_brush(user.state->core, id).build(),
        UiMakeBrush::make_solid(0xFFFFFF00));

    user.end_generic();
}

void img(UiPass &user, UiImageId id, Vector2 scale)
{
    AutoLayoutElement el = {};
    el.layout = {AutoLayout::row};
    Vector2 size = vector2i_to_vector2(user.state->core->get_image(id)->size);

    el.width = {AutoLayoutDimension::pixel, size.x * scale.x};
    el.height = {AutoLayoutDimension::pixel, size.y * scale.y};

    user.begin_generic(
        el, UiMakeBrush::make_image_brush(user.state->core, id).build(),
        UiMakeBrush::make_solid(0xFFFFFF00));

    user.end_generic();
}

void label(UiPass &user, const char *text, Vector2 scale, UiBrush style)
{
    Vector2 size = user.state->font.bounds_text_utf8({0, 0}, text, scale).siz;
    if (user.bold)
    {
        size = user.state->font_bold.bounds_text_utf8({0, 0}, text, scale).siz;
    }

    void *ptr = user.state->frame_storage.alloc(strlen(text) + 1);
    strcpy((char *)ptr, text);

    AutoLayoutElement el = {};
    el.layout = {AutoLayout::row};
    el.width = {AutoLayoutDimension::pixel, size.x};
    el.height = {AutoLayoutDimension::pixel, size.y};

    UiGenericStyles styles = {style, {}, (char *)ptr, scale, 0};
    styles.bold = user.bold;
    styles.scale = 1;

    el.userdata = user.state->frame_storage.alloct<UiGenericStyles>();
    *(UiGenericStyles *)el.userdata = styles;

    user.layout.add_element(user.current_node, el);
}

void label_wrapped(UiPass &user, const char *text, float max_width,
                   Vector2 scale, UiBrush style)
{
    char buf[1024];
    buf[0] = 0;
    size_t buf_len = 0;

    float line_width = 0;
    const char *p = text;

    while (*p)
    {
        // Find next word
        const char *word_start = p;
        while (*p && *p != ' ' && *p != '\n')
            p++;

        size_t word_len = p - word_start;
        char word[256];
        if (word_len >= sizeof(word))
            word_len = sizeof(word) - 1;
        memcpy(word, word_start, word_len);
        word[word_len] = 0;

        float word_width =
            user.state->font.bounds_text_utf8({0, 0}, word, scale).siz.x;
        float space_width =
            user.state->font.bounds_text_utf8({0, 0}, " ", scale).siz.x;

        if (line_width > 0 && line_width + space_width + word_width > max_width)
        {
            if (buf_len < sizeof(buf) - 1)
                buf[buf_len++] = '\n';
            line_width = 0;
        }
        else if (line_width > 0)
        {
            if (buf_len < sizeof(buf) - 1)
                buf[buf_len++] = ' ';
            line_width += space_width;
        }

        for (size_t i = 0; i < word_len && buf_len < sizeof(buf) - 1; i++)
            buf[buf_len++] = word[i];
        line_width += word_width;

        // Skip spaces
        while (*p == ' ')
            p++;
        if (*p == '\n')
        {
            if (buf_len < sizeof(buf) - 1)
                buf[buf_len++] = '\n';
            line_width = 0;
            p++;
        }
    }
    buf[buf_len] = 0;

    label(user, buf, scale, style);
}

bool input(UiPass &user, const char *id, char *out, int max)
{
    bool edited = false;

    user.state->element_storage.push(id, {true});

    if (user.focused())
    {
        for (int i = 0; i < user.state->input.input_len; i++)
        {
            if (user.state->input.input[i] == '\b')
            {
                if (strlen(out) > 0)
                {
                    out[strlen(out) - 1] = 0;
                    edited = true;
                }
            }
            else if (user.state->input.input[i])
            {
                if (strlen(out) < max - 1)
                {
                    out[strlen(out)] = user.state->input.input[i];
                    edited = true;
                }
            }
        }
    }

    AutoLayoutElement el = {};
    el.layout = {AutoLayout::row};
    el.width = {AutoLayoutDimension::pixel, 150};
    el.height = {AutoLayoutDimension::pixel, 20};
    el.clip = true;
    el.padding = {2, 2, 2, 2};
    el.margin = {0, 0, 0, 0};
    el.border = {1, 1, 1, 1};

    UiBrush bg = UiMakeBrush::make_gradient(theme[0], theme[1]);
    bg.color_top.a = 0x66;
    UiBrush border = UiMakeBrush::make_gradient(0x00000088, 0x00000088);

    if (user.focused())
    {
        bg = UiMakeBrush::make_gradient(theme[5], theme[4]);
        el.width = {AutoLayoutDimension::autom};
        el.height = {AutoLayoutDimension::autom};
    }

    user.begin_generic(el, bg, border, user.state->element_storage.id());

    char *t = (char *)user.state->frame_storage.alloc(strlen(out) + 2);
    memcpy(t, out, strlen(out));
    t[strlen(out)] = user.focused() ? '|' : 0;
    t[strlen(out) + 1] = 0;

    label(user, t, {1.2, 1.2}, UiMakeBrush::make_solid(theme[2]));

    user.end_generic();

    user.state->element_storage.pop();

    return edited;
}

AutoLayoutElement make_element(AutoLayout layout, Vector2 size, bool autox,
                               bool autoy, Vector2 align = {0, 0}, float p = 3,
                               float m = 0);

AutoLayoutElement make_auto(AutoLayout layout, Vector2 align = {0, 0});

static void msgbox_icon(UiPass &user, uint32_t color1, uint32_t color2,
                        const char *text)
{
    UiBrush border = UiMakeBrush::make_gradient(0x00000088, 0x00000000);
    user.begin_generic(make_element({AutoLayout::column}, {40, 40}, false,
                                    false, {0.5, 0.5}, 1, 1),
                       UiMakeBrush::make_gradient(color1, color2), border);
    label(user, text, {2, 2}, UiMakeBrush::make_solid(0xFFFFFFFF));
    user.end_generic();
}

int msgbox(UiPass &user, const char *title, const char *text, MsgBoxType type,
           const char *buttons[])
{
    Rect scaled_screen_rect = Rect{
        {0, 0}, Vector2{sapp_widthf(), sapp_heightf()} / user.state->dpi_scale};

    int result = -1;

    WindowInfo winfo = {
        title, rect_center_rect(scaled_screen_rect, {0, 0, 450, 200}), true};
    window(user, winfo, [&] {
        user.begin_generic(make_auto({AutoLayout::column}, {0, 0}), {}, {});

        user.begin_generic(make_auto({AutoLayout::row}), {}, {});

        switch (type)
        {
        default:
        case MsgBoxType::Info:
            msgbox_icon(user, 0x4444FFFF, 0x0033FF66, "?");
            break;
        case MsgBoxType::Warning:
            msgbox_icon(user, 0xFFCC44FF, 0xFFAA0066, ":/");
            break;
        case MsgBoxType::Error:
            msgbox_icon(user, 0xFF4444FF, 0xFF000066, "!");
            break;
        }

        label_wrapped(user, text, 400, {1.5, 1.5});

        user.end_generic();
        user.begin_generic(make_auto({AutoLayout::row}), {}, {});

        for (int i = 0; buttons[i]; i++)
        {
            if (button(user, buttons[i]))
            {
                result = i;
            }
        }

        user.end_generic();
        user.end_generic();
    });

    return result;
}
