#pragma once

#include "catedu/rendering/color.hpp"
#include "catedu/ui/user.hpp"
#include <functional>

struct UX
{
    UiState *state;
    UiPass pass;

    // NOTE: These are the properties that are going to be applied to the next
    // element
    UiBrush nx_background;
    UiBrush nx_border;
    UiBrush nx_color;
    float nx_border_size;
    float nx_margin;
    float nx_padding;

    static UX begin(UiState &state);
    void end();

    // MARK: Events
    bool clicked();

    // MARK: Style
    UX &background_color(Color color);
    UX &border_size(float px);
    UX &border_color(Color color);
    UX &color(Color color);
    UX &margin(float px);
    UX &padding(float px);

    // MARK: Layout
    UX &row(std::function<void()> cb);
    UX &column(std::function<void()> cb);

    // MARK: Widgets
    void dropdown(const char *label, const char *options[], int count,
                  int &selction);
    bool button(const char *text);
    bool input(const char *label, char *buffer, size_t buffer_size);
    UX &label(const char *text);
    UX &heading(const char *text, int size = 0);
};
