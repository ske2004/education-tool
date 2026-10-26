#include "ux.hpp"
#include "catedu/ui/layout/autolayout.hpp"
#include "catedu/ui/rendering/make_brush.hpp"
#include "catedu/ui/widgets.hpp"
#include <cstdio>
#include <cstring>

UX UX::begin(UiState &state)
{
    UX ux = {};
    ux.state = &state;
    ux.pass = UiPass::init(state);
    ux.pass.begin_pass();
    ux.nx_color = UiMakeBrush::make_solid(0xFFFFFFFF);

    return ux;
}

void UX::end()
{
    pass.end_pass();
}

// EVENTS:
bool UX::clicked()
{
    return pass.state->input.k[INPUT_MB_LEFT].pressed && pass.active();
}

// MARK: Style
#pragma region style
UX &UX::background_color(Color color)
{
    nx_background = UiMakeBrush::make_solid(color);
    return *this;
}

UX &UX::border_size(float px)
{
    nx_border_size = px;
    return *this;
}

UX &UX::border_color(Color color)
{
    nx_border = UiMakeBrush::make_solid(color);
    return *this;
}

UX &UX::color(Color color)
{
    nx_color = UiMakeBrush::make_solid(color);
    return *this;
}

UX &UX::margin(float px)
{
    nx_margin = px;
    return *this;
}

UX &UX::padding(float px)
{
    nx_padding = px;
    return *this;
}
#pragma endregion

// MARK: Layout
#pragma region layout
UX &UX::row(std::function<void()> cb)
{
    // TODO: Add check if element is hidden, and don't render it.

    AutoLayoutElement el = {};
    el.border = {nx_border_size, nx_border_size, nx_border_size,
                 nx_border_size};
    el.margin = {nx_margin, nx_margin, nx_margin, nx_margin};
    el.padding = {nx_padding, nx_padding, nx_padding, nx_padding};
    el.layout.type = AutoLayout::row;
    pass.begin_generic(el, nx_background, nx_border);
    nx_background = {};
    nx_border = {};
    nx_border_size = {};
    nx_margin = {};
    nx_padding = {};
    cb();
    pass.end_generic();
    return *this;
}

UX &UX::column(std::function<void()> cb)
{
    // TODO: Add check if element is hidden, and don't render it.

    AutoLayoutElement el = {};
    el.border = {nx_border_size, nx_border_size, nx_border_size,
                 nx_border_size};
    el.margin = {nx_margin, nx_margin, nx_margin, nx_margin};
    el.padding = {nx_padding, nx_padding, nx_padding, nx_padding};
    el.layout.type = AutoLayout::column;
    pass.begin_generic(el, nx_background, nx_border);
    nx_background = {};
    nx_border = {};
    nx_border_size = {};
    nx_margin = {};
    nx_padding = {};
    cb();
    pass.end_generic();
    return *this;
}
#pragma endregion

// MARK: Widgets
#pragma region widgets
void UX::dropdown(const char *label, const char *options[], int count,
                  int &selction)
{
    if (count <= 0 || options == nullptr)
    {
        return;
    }

    char id[128];
    snprintf(id, sizeof(id), "%s##dropdown", label ? label : "dd");
    state->element_storage.push(id, {});
    UiPersistentElement *pe = state->element_storage.value();

    if (selction < 0)
    {
        selction = 0;
    }
    if (selction >= count)
    {
        selction = count - 1;
    }

    char btn_text[256];
    if (label && label[0] != '\0')
    {
        snprintf(btn_text, sizeof(btn_text), "%s: %s", label, options[selction]);
    }
    else
    {
        snprintf(btn_text, sizeof(btn_text), "%s", options[selction]);
    }

    if (::button(pass, btn_text))
    {
        pe->pinned = !pe->pinned;
    }

    if (pe->pinned)
    {
        for (int i = 0; i < count; i++)
        {
            if (::button(pass, options[i]))
            {
                selction = i;
                pe->pinned = false;
            }
        }
    }

    state->element_storage.pop();
}

bool UX::button(const char *text)
{
    // TODO: Remove the widgets from external access once everything is using
    // this UX structure.
    return ::button(pass, text);
}

bool UX::input(const char *label, char *buffer, size_t buffer_size)
{
    return ::input(this->pass, label, buffer, (int)buffer_size);
}

UX &UX::label(const char *text)
{
    ::label(pass, text, {1, 1}, nx_color);
    nx_color = UiMakeBrush::make_solid(0xFFFFFFFF);

    return *this;
}

UX &UX::heading(const char *text, int scale)
{
    pass.bold = true;
    ::label(pass, text, {2 + (float)scale, 2 + (float)scale}, nx_color);
    nx_color = UiMakeBrush::make_solid(0xFFFFFFFF);
    pass.bold = false;

    return *this;
}
#pragma endregion
