// #pragma once
#include "catedu/ui/user.hpp"
#include <functional>

struct WindowInfo
{
    const char *title;
    Rect rect;
    bool if_static;
};

enum struct MsgBoxType
{
    Info,
    Warning,
    Error
};

bool begin_show_window(UiPass &user, WindowInfo info);
void end_show_window(UiPass &user);
static inline void window(UiPass &user, WindowInfo info,
                          std::function<void()> cb)
{
    if (begin_show_window(user, info))
    {
        cb();
    }
    end_show_window(user);
}

bool input(UiPass &user, const char *id, char *out, int max);

void begin_button_frame(UiPass &user, const char *id, AutoLayoutElement el = {},
                        Vector4 background = {1.0, 1.0, 1.0, 1.0},
                        float radius = 1);
bool end_button_frame(UiPass &user);

bool button(UiPass &user, const char *text,
            Vector4 background = {1.0, 1.0, 1.0, 1.0});
bool button_toggle(UiPass &user, const char *text, bool &state);

bool button_radio(UiPass &user, const char *text, int &mode, int val);

void img(UiPass &user, const char *path, Vector2 scale = {1, 1});
void img(UiPass &user, UiImageId id, Vector2 scale = {1, 1});
void label(UiPass &user, const char *text, Vector2 scale = {1, 1},
           UiBrush style = {{0, 0, 0, 255}, {0, 0, 0, 255}});
void label_wrapped(UiPass &user, const char *text, float max_width,
                   Vector2 scale = {1, 1},
                   UiBrush style = {{0, 0, 0, 255}, {0, 0, 0, 255}});

/// @returns Which button was pressed. -1 if none.
int msgbox(UiPass &user, const char *title, const char *text, MsgBoxType type,
           const char *buttons[]);
