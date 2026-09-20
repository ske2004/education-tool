#include "main_menu.hpp"
#include "catedu/gui/editor/camera.hpp"
#include "catedu/gui/transition/transition.hpp"
#include "catedu/scene/render_world.hpp"
#include "catedu/ui/rendering/core.hpp"
#include "catedu/ui/rendering/make_brush.hpp"
#include "catedu/ui/ux.hpp"
#include "catedu/ui/widgets.hpp"
#include "sokol/sokol_app.h"

AutoLayoutElement make_element(AutoLayout layout, Vector2 size, bool autox,
                               bool autoy, Vector2 align, float p = 3,
                               float m = 0)
{
    AutoLayoutElement el = {};
    el.layout = layout;
    el.width = {autox ? AutoLayoutDimension::autom : AutoLayoutDimension::pixel,
                size.x};
    el.height = {autoy ? AutoLayoutDimension::autom
                       : AutoLayoutDimension::pixel,
                 size.y};
    el.padding = {p, p, p, p};
    el.margin = {m, m, m, m};
    el.border = {p, p, p, p};
    el.align_width = align.x;
    el.align_height = align.y;

    return el;
}

AutoLayoutElement make_auto(AutoLayout layout, Vector2 align = {})
{
    AutoLayoutElement el = {};
    el.layout = layout;
    el.align_width = align.x;
    el.align_height = align.y;
    return el;
}

GuiMainMenu GuiMainMenu::init(UiState *ui_state)
{
    GuiMainMenu mm = {normal, ui_state};
    mm.camera = EditorCamera::create();
    return mm;
}

void GuiMainMenu::deinit()
{
    // Nothing yet
}

static bool big_menu_button(UiPass &user, const char *text, bool major = false)
{
    float size = major ? 256 : 192;
    float margin = (256 - size) / 2;

    AutoLayoutElement el = make_element({AutoLayout::row}, {size, size}, false,
                                        false, {0.5, 0.5}, 0, margin);
    el.border = {2, 2, 2, 2};
    begin_button_frame(user, text, el, {1, 1, 1, 1}, 0.5);
    label(user, text, {3, 3});
    return end_button_frame(user);
}

static void menu_settings(UiPass &pass, float &scale_delta, GuiMainMenu &state)
{
    const char *buttons[] = {"Close", "Scale+", "Scale-", NULL};
    switch (msgbox(pass, "Settings", "Change the scale of the UI.",
                   MsgBoxType::Info, buttons))
    {
    case 0:
        state.state = GuiMainMenu::normal;
        break;
    case 1:
        scale_delta = 0.1;
        break;
    case 2:
        scale_delta = -0.1;
        break;
    }
}

static void menu_exit(UiPass &pass, GuiMainMenu &state)
{
    const char *buttons[] = {"Yes", "No", NULL};
    switch (msgbox(pass, "Exit", "Are you sure you want to exit?",
                   MsgBoxType::Warning, buttons))
    {
    case 0:
        sapp_request_quit();
        break;
    case 1:
        state.state = GuiMainMenu::normal;
        break;
    }
}

bool GuiMainMenu::show(UX &ux, GuiTransition &transition, World &world,
                       Renderer &renderer, ResourceSpec &resources)
{
    UiPass &pass = ux.pass;

    this->angle += sapp_frame_duration() * 10.0f;
    this->camera.follow({0, 0, -10}, this->angle, 6);
    this->camera.update({sapp_width(), sapp_height()});
    renderer.camera = camera.cam;
    renderer.begin_pass();
    render_place(*world.first, renderer, resources, 12.0f);
    renderer.end_pass();

    pass.begin_generic(
        make_element({AutoLayout::column},
                     {sapp_widthf() / this->ui_state->dpi_scale,
                      sapp_heightf() / this->ui_state->dpi_scale},
                     false, false, {0.5, 0.5}, 0),
        UiMakeBrush::make_gradient(0x00004499, 0x000A8899), {});

    ux.color(0x44CC00FF).heading("STORYLAND", 3);

    ux.row([&] {
        if (big_menu_button(pass, "Settings"))
        {
            this->state = GuiMainMenu::settings;
        }
        if (big_menu_button(pass, "Play & Edit", true))
        {
            transition.begin();
        }
        if (big_menu_button(pass, "Exit"))
        {
            this->state = GuiMainMenu::exit;
        }
    });

    pass.end_generic();

    float scale_delta = 0;
    switch (this->state)
    {
    case GuiMainMenu::settings:
        menu_settings(pass, scale_delta, *this);
        break;
    case GuiMainMenu::exit:
        menu_exit(pass, *this);
        break;
    default:
        break;
    }

    // FIXME: This will scale while the UI User is in the pass, but it will not
    // be applied until the next frame. This is a bug.
    pass.state->dpi_scale += scale_delta;

    return transition.switching();
}
