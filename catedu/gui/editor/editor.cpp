#include "editor.hpp"
#include "catedu/core/storage/stack.hpp"
#include "catedu/genobj/building.hpp"
#include "catedu/genobj/deleter.hpp"
#include "catedu/genobj/render.hpp"
#include "catedu/genobj/tree.hpp"
#include "catedu/gui/transition/transition.hpp"
#include "catedu/rendering/3d/pbr.hpp"
#include "catedu/scene/render_world.hpp"
#include "catedu/scene/world_file.hpp"
#include "catedu/sys/input.hpp"
#include "catedu/ui/layout/autolayout.hpp"
#include "catedu/ui/rendering/core.hpp"
#include "catedu/ui/rendering/make_brush.hpp"
#include "catedu/ui/widgets.hpp"
#include "edit_building.hpp"
#include "offscreen.hpp"
#include <catedu/genobj/ground.hpp>
#include <catedu/genobj/player.hpp>
#include <catedu/genobj/road.hpp>
#include <catedu/genobj/wall.hpp>
#include <cstdio>
#include <umka_api.h>

bool icon_button(UiPass &user, const char *name, const char *icon,
                 Vector4 color = {1.0, 1.0, 1.0, 1.0}, float scale = 1);

AutoLayoutElement create_main_element(UiPass &user)
{
    AutoLayoutElement element = {};
    element.position = AutoLayoutPosition::absolute;
    element.width.type = AutoLayoutDimension::pixel;
    element.width.value = sapp_widthf() / user.state->dpi_scale;
    element.height.type = AutoLayoutDimension::pixel;
    element.height.value = sapp_heightf() / user.state->dpi_scale;
    return element;
}

enum class RectSide
{
    Left,
    Right,
    Top,
    Bottom
};

bool rect_side_is_horizontal(RectSide &side)
{
    return side == RectSide::Bottom || side == RectSide::Top;
}

void begin_toolbar(UiPass &user, const char *name, float align_y, float align_x)
{
    AutoLayoutElement element = create_main_element(user);
    element.align_height = align_y;
    element.align_width = align_x;
    user.begin_generic(element, {}, {});
    user.push_id(name);

    AutoLayoutElement toolbar = {};
    toolbar.layout.type = AutoLayout::row;
    toolbar.width.type = AutoLayoutDimension::autom;
    toolbar.height.type = AutoLayoutDimension::autom;
    toolbar.padding = {2, 2, 2, 2};
    toolbar.margin = {2, 2, 2, 2};
    toolbar.border = {0, 0, 0, 0};
    toolbar.align_height = 1;
    user.begin_generic(toolbar, {},
                       UiMakeBrush::make_plain_brush()
                           .squircle(0.9, 0.5)
                           .with_gradient(0x00000077, 0x00000000)
                           .build(),
                       user.state->element_storage.id());
}

void end_toolbar(UiPass &user)
{
    user.end_generic();
    user.pop_id();
    user.end_generic();
}

bool icon_button(UiPass &user, const char *name, const char *icon,
                 Vector4 color, float scale)
{
    AutoLayoutElement el = {};
    el.padding = {2, 2, 2, 2};
    el.margin = {2, 2, 2, 2};
    el.border = {1, 1, 1, 1};

    begin_button_frame(user, name, el, color, 0.5);
    img(user, icon, {scale / 1.2f, scale / 1.2f});
    return end_button_frame(user);
}

bool object_icon_button(UiPass &user, const char *name, SubEditor::Type type,
                        SubEditor::Type &current, Renderer &renderer,
                        ResourceSpec &resources)
{
    AutoLayoutElement el = {};
    el.width = {AutoLayoutDimension::pixel, 90};
    el.height = {AutoLayoutDimension::pixel, 75};
    el.align_width = 0.5;
    el.align_height = 0.5;
    el.padding = {2, 2, 10, 2};
    el.margin = {2, 2, 2, 2};
    el.border = {1, 1, 1, 1};

    UiImageId id;

    Vector4 color = {1.0, 1.0, 1.0, 1.0};
    if (current == type)
    {
        color = {0.4, 1.0, 0.4, 1.0};
    }

    begin_button_frame(user, name, el, color, 0.5);
    {
        Camera camera = Camera::init(5);
        camera.set_aspect(el.width.value / el.height.value);

        if (type == SubEditor::Type::building)
        {
            camera.move(0, 0, -20);
            camera.rotate_around({0, 0, 0}, 45, -45);
            camera.move(0, 0.5, 0);
        }
        else if (type == SubEditor::Type::wall)
        {
            camera.move(0, 0, -5);
            camera.rotate_around({0, 0, 0}, 45, -45);
            camera.move(0, 0.1, 0);
        }
        else
        {
            camera.move(0, 0, -10);
            camera.rotate_around({0, 0, 0}, 45, -45);
            camera.move(0, 0.1, 0);
        }

        renderer.camera = camera;
        renderer.begin_pass_offscreen(offscreen_pass_action(),
                                      offscreen_alloc(user.state->core, id));

        GeneratedObject obj;
        switch (type)
        {
        case SubEditor::Type::deleter:
            obj = genmesh_generate_deleter();
            break;
        case SubEditor::Type::building:
            obj = genmesh_generate_building(5);
            break;
        case SubEditor::Type::road:
            obj = genmesh_generate_road();
            break;
        case SubEditor::Type::player:
            obj = genmesh_generate_player();
            break;
        case SubEditor::Type::wall:
            obj = genmesh_generate_wall();
            break;
        case SubEditor::Type::tree:
            obj = genmesh_generate_tree();
            break;
        }
        genobj_render_object(renderer, get_genres(resources), obj,
                             Matrix4::scale(0.1f));

        renderer.end_pass();
    }

    img(user, id, {0.5, 0.5});
    label(user, name, {1.2, 1.2});
    if (end_button_frame(user))
    {
        current = type;
        return true;
    }
    return false;
}

GuiEditor GuiEditor::init(UiState *ui_state)
{
    GuiEditor result = {};
    result.ui_state = ui_state;
    result.dispatcher = WorldFile::load("assets/world.dat");
    result.editor_camera = EditorCamera::create();
    result.previous_place = result.dispatcher.world.current;
    result.script_editor = ScriptEditor::create(result.dispatcher.world.script);

    printf("Init editor\n");

    offscreen_init_targets();

    return result;
}

void show_script_panel(UiPass &user, GuiEditor &editor)
{
    AutoLayoutElement el = {};
    el.layout = {AutoLayout::row};
    el.padding = {1, 1, 1, 1};

    user.begin_generic(el, {}, {});

    editor.script_editor.show_palette(user);
    editor.script_editor.show(user);

    user.end_generic();
}

void show_character_panel(UiPass &user, GuiEditor &editor,
                          ResourceSpec &resources, Renderer &renderer)
{
    object_icon_button(user, "Player", SubEditor::Type::player,
                       editor.sub_editor.type, renderer, resources);
}

void show_build_panel(UiPass &user, GuiEditor &editor, ResourceSpec &resources,
                      Renderer &renderer)
{
    struct ObjElement
    {
        const char *name;
        SubEditor::Type type;
    };

    Stack<ObjElement> elements = {};

    elements.push({"Delete", SubEditor::Type::deleter});

    if (editor.dispatcher.world.first != editor.dispatcher.world.current)
    {
        if (editor.sub_editor.type == SubEditor::Type::building)
        {
            editor.sub_editor.type = SubEditor::Type::wall;
        }
    }
    else
    {
        elements.push({"Building", SubEditor::Type::building});
        elements.push({"Road", SubEditor::Type::road});
        elements.push({"Tree", SubEditor::Type::tree});
    }

    elements.push({"Wall", SubEditor::Type::wall});

    for (int i = 0; i < elements.count / 2 + 1; i++)
    {
        AutoLayoutElement el = {};
        el.layout.type = AutoLayout::row;
        user.begin_generic(el, {}, {});
        for (int j = 0; j < 2; j++)
        {
            if (i * 2 + j >= elements.count)
            {
                break;
            }
            ObjElement el = elements[i * 2 + j];
            object_icon_button(user, el.name, el.type, editor.sub_editor.type,
                               renderer, resources);
        }
        user.end_generic();
    }

    elements.deinit();
}

void show_control_panel(UiPass &user, GuiEditor &editor,
                        ResourceSpec &resources, Renderer &renderer)
{
    SubMode nextmode = editor.sub_mode;
    begin_toolbar(user, "Panel selector", 1.0, 1.0);
    {
        AutoLayoutElement el = {};
        el.align_width = 1.0;
        user.begin_generic(el, {}, {});

        const char *lbl = "[error]";
        switch (editor.sub_mode)
        {
        case SubMode::script:
            lbl = "Script";
            break;
        case SubMode::character:
            lbl = "Character";
            break;
        case SubMode::build:
            lbl = "Build";
            break;
        }

        label(user, lbl, {1.5, 1.5}, UiMakeBrush::make_solid(0xFFFFFFFF));

        AutoLayoutElement row = {};
        row.layout.type = AutoLayout::row;
        row.align_height = 1.0;
        user.begin_generic(row, {}, {});

        auto btn = [&](const char *name, const char *img, SubMode mode) {
            if (editor.sub_mode == mode)
            {
                if (icon_button(user, name, img, {0.8, 1.0, 0.8, 1.0}, 1.2))
                {
                    nextmode = mode;
                }
            }
            else
            {
                if (icon_button(user, name, img))
                {
                    nextmode = mode;
                }
            }
        };

        btn("Script", "assets/gui/script.png", SubMode::script);
        btn("Character", "assets/gui/char.png", SubMode::character);
        btn("Build", "assets/gui/building.png", SubMode::build);
        user.end_generic();

        user.end_generic();
    }
    end_toolbar(user);

    AutoLayoutElement parent = {};
    parent.align_width = 1.0;
    parent.width = {AutoLayoutDimension::pixel,
                    sapp_widthf() / user.state->dpi_scale};
    user.begin_generic(parent, {}, {});

    AutoLayoutElement element = {};
    element.clip = true;
    element.padding = {2, 2, 2, 2};
    element.width = {AutoLayoutDimension::pixel, 202};
    if (editor.sub_mode == SubMode::script)
    {
        element.width = {AutoLayoutDimension::pixel, 202 * 2 + 2};
    }

    element.height = {AutoLayoutDimension::pixel,
                      (sapp_heightf() - 100) / user.state->dpi_scale};
    user.state->element_storage.push("Left Panel", {});
    element.scroll = user.state->element_storage.value()->scroll;
    user.begin_generic(element, {},
                       UiMakeBrush::make_plain_brush()
                           .squircle(0.8, 0.9)
                           .with_gradient(0x00002200, 0x00000077)
                           .build(),
                       user.state->element_storage.id(), 1, true);

    if (editor.dispatcher.world.current != editor.dispatcher.world.first)
    {
        if (button(user, "Back outside"))
        {
            editor.dispatcher.world.current = editor.dispatcher.world.first;
            editor.dispatcher.world.script->acquire_start_event();
        }
    }

    switch (editor.sub_mode)
    {
    case SubMode::script:
        show_script_panel(user, editor);
        break;
    case SubMode::character:
        show_character_panel(user, editor, resources, renderer);
        break;
    case SubMode::build:
        show_build_panel(user, editor, resources, renderer);
        break;
    }

    if (user.hovered())
    {
        user.state->element_storage.value()->scroll.y +=
            user.state->input.mouse_wheel * 20;

        if (user.state->element_storage.value()->scroll.y > 0)
        {
            user.state->element_storage.value()->scroll.y = 0;
        }
    }

    user.end_generic();
    user.state->element_storage.pop();
    user.end_generic();

    editor.sub_mode = nextmode;
}

void show_popups(UiPass &user, GuiEditor &editor, bool &return_back)
{
    if (editor.exit_requested)
    {
        const char *options[] = {"Yes", "No", NULL};
        switch (msgbox(user, "Exit requested",
                       "Are you sure you want to exit?\nAll unsaved changes "
                       "will be lost.",
                       MsgBoxType::Warning, options))
        {
        case 0:
            editor.exit_requested = false;
            editor.dispatcher.dirty = false;
            sapp_request_quit();
            break;
        case 1:
            editor.exit_requested = false;
            break;
        }
    }

    if (editor.playtest_no_player)
    {
        return_back = false;
        const char *options[] = {"Ok", NULL};
        switch (msgbox(user, "Error", "Add a player to the scene",
                       MsgBoxType::Error, options))
        {
        case 0:
            editor.playtest_no_player = false;
            break;
        }
    }

    if (return_back)
        editor.tried_to_return_back = true;

    if (editor.tried_to_return_back && editor.dispatcher.dirty)
    {
        return_back = false;
        const char *options[] = {"Yes", "No", NULL};
        switch (msgbox(user, "Return to main menu",
                       "Are you sure you want to return to the main menu?\nAll "
                       "unsaved changes will be lost.",
                       MsgBoxType::Warning, options))
        {
        case 0:
            return_back = true;
            editor.dispatcher.dirty = false;
            break;
        case 1:
            editor.tried_to_return_back = false;
            break;
        }
    }
}

void show_editor_controls(UiPass &user, GuiEditor &editor, bool &return_back)
{
    begin_toolbar(user, "Controls", 1.0, 0.0);

    if (!editor.playtesting)
    {
        if (icon_button(user, "Playtest", "assets/gui/play.png", {1, 1, 1, 1},
                        1.1))
        {
            editor.playtesting = true;
            editor.playtest = Playtest::create(editor.dispatcher.world.clone());
            // FIXME: There's probably better ways to handle this
            if (editor.playtest.player == NULL_ID)
            {
                editor.playtest.destroy();
                editor.playtesting = false;
                editor.playtest_no_player = true;
            }
        }

        if (icon_button(user, "Undo", "assets/gui/undo.png"))
        {
            editor.dispatcher.undo();
        }

        if (icon_button(user, "Redo", "assets/gui/redo.png"))
        {
            editor.dispatcher.redo();
        }

        if (icon_button(user, "Home", "assets/gui/home.png"))
        {
            return_back = true;
        }

        Vector4 color = {1.0, 1.0, 1.0, 1.0};
        if (editor.dispatcher.dirty)
        {
            color = {1.0, 0.8, 0.8, 1.0};
        }

        if (icon_button(user, "Save", "assets/gui/save.png", color))
        {
            WorldFile::save("assets/world.dat", editor.dispatcher);
        }
    }
    else
    {
        if (icon_button(user, "Stop Playtest", "assets/gui/stop.png",
                        {1, 1, 1, 1}, 1.1))
        {
            // TODO: Find a better way of handling playtest memory.
            editor.playtesting = false;
            editor.playtest.destroy();
            editor.playtest = {};
        }
    }

    end_toolbar(user);
}

void handle_shortcuts(GuiEditor &editor, Input &input)
{
    if (input.shortcut(MOD_CTRL, SAPP_KEYCODE_S))
    {
        WorldFile::save("assets/world.dat", editor.dispatcher);
    }

    if (input.shortcut(MOD_CTRL, SAPP_KEYCODE_Z))
    {
        editor.dispatcher.undo();
    }

    if (input.shortcut(MOD_CTRL, SAPP_KEYCODE_Y))
    {
        editor.dispatcher.redo();
    }
}

void render_physics_boxes(Renderer &renderer, PhysicsWorld &world,
                          ResourceSpec &resources)
{
    Model *hitbox =
        &resources.models.get(resources.find_model_by_name("hitbox"))->model;

    for (auto [id, body] : iter(world.bodies))
    {
        Params vs_params;
        vs_params.model =
            Matrix4::translate({body.area.pos.x + body.area.siz.x / 2.0f, 0,
                                body.area.pos.y + body.area.siz.y / 2.0f}) *
            Matrix4::scale_v({body.area.siz.x, 1.0f, body.area.siz.y});
        vs_params.lightness = 1.0f;
        vs_params.color_mul = {1.0f, 1.0f, 1.0f, 1.0f};
        renderer.render_model(*hitbox, vs_params);
    }
}

void show_editor_ui(GuiEditor &editor, UiPass &user, ResourceSpec &resources,
                    Renderer &renderer, Input &input, bool &return_back,
                    GuiTransition &transition)
{
    renderer.camera = editor.editor_camera.cam;
    renderer.begin_pass();

    GenResources gen_resources = get_genres(resources);

    if (user.actively_hovered() && !editor.playtesting &&
        editor.sub_mode != SubMode::script)
    {
        sapp_lock_mouse(input.k[INPUT_MB_MIDDLE].held);
        if (input.k[INPUT_MB_MIDDLE].held)
        {
            editor.editor_camera.handle_controls(input);
        }
        else
        {
            editor.sub_editor.update(editor.dispatcher, input,
                                     editor.editor_camera.cam);
            editor.sub_editor.render(user, renderer, editor.dispatcher,
                                     gen_resources);
            editor.editor_camera.handle_controls(input);
        }
    }
    editor.editor_camera.update({sapp_width(), sapp_height()});

    if (editor.playtesting)
    {
        input.mouse_wheel = 0;
        editor.playtest.update(user, input, editor.editor_camera, transition);
        editor.editor_camera.handle_controls(input);
    }

    if (editor.previous_place != editor.dispatcher.world.current)
    {
        if (editor.dispatcher.world.current == editor.dispatcher.world.first)
        {
            editor.script_editor.current =
                editor.dispatcher.world.script->acquire_start_event();
        }
        else
        {
            editor.script_editor.current =
                editor.dispatcher.world.script->acquire_place_event(
                    editor.dispatcher.world.current);
        }

        transition.begin();
    }

    Place *place = (transition.going() && !editor.playtesting)
                       ? editor.previous_place
                       : (!editor.playtesting ? editor.dispatcher.world.current
                                              : editor.playtest.world.current);

    render_place(*place, renderer, resources);

    renderer.end_pass();

    if (!editor.playtesting)
    {
        show_control_panel(user, editor, resources, renderer);
    }

    show_editor_controls(user, editor, return_back);

    handle_shortcuts(editor, input);
}

bool show_main_editor(GuiEditor &editor, UiPass &user, ResourceSpec &resources,
                      Renderer &renderer, Input &input,
                      GuiTransition &transition)
{
    AutoLayoutElement element = create_main_element(user);
    user.state->element_storage.push("Main", {});
    user.begin_generic(element, {}, {}, user.state->element_storage.id());

    bool return_back = false;
    show_editor_ui(editor, user, resources, renderer, input, return_back,
                   transition);
    user.end_generic();
    user.state->element_storage.pop();

    return return_back;
}

bool GuiEditor::show(UiPass &user, GuiTransition &transition,
                     Renderer &renderer, ResourceSpec &resources)
{
    offscreen_clear();

    bool return_back = show_main_editor(*this, user, resources, renderer,
                                        user.state->input, transition);

    if (this->previous_place != this->dispatcher.world.current)
    {
        transition.begin();
    }

    if (transition.switching() && !this->playtesting)
    {
        this->previous_place = this->dispatcher.world.current;
        float angle = 0;
        if (this->dispatcher.world.current->interior)
        {
            angle = MATH_TAU / 2;
        }
        this->editor_camera.lockin({0, 0, 0}, angle);
        std::swap(this->previous_place, this->dispatcher.world.current);
    }

    show_popups(user, *this, return_back);

    if (return_back)
    {
        transition.begin();
        this->returning_back = true;
        return_back = false;
    }

    if (this->returning_back && transition.switching())
    {
        return_back = true;
    }

    return return_back;
}

void GuiEditor::deinit()
{
    if (playtesting)
    {
        playtest.destroy();
    }
    offscreen_deinit_targets(this->ui_state->core);
    dispatcher.destroy();
}

void SubEditor::update(Dispatcher &disp, Input &input, Camera &camera)
{
    switch (type)
    {
    case Type::deleter:
        edit_delete.update(disp, input, camera);
        break;
    case Type::building:
        edit_building.update(disp, input, camera);
        break;
    case Type::road:
        edit_line.update(disp, input, camera, Object::Type::road);
        break;
    case Type::wall:
        edit_line.update(disp, input, camera, Object::Type::wall);
        break;
    case Type::player:
        edit_basic.update(disp, input, camera, Object::Type::player);
        break;
    case Type::tree:
        edit_basic.update(disp, input, camera, Object::Type::tree);
        break;
    }
}

void SubEditor::render(UiPass &user, Renderer &renderer, Dispatcher &disp,
                       GenResources &gen_resources)
{
    switch (type)
    {
    case Type::deleter:
        edit_delete.render(renderer, gen_resources);
        break;
    case Type::building:
        edit_building.render(renderer, gen_resources);
        break;
    case Type::road:
        edit_line.render(renderer, disp, gen_resources, Object::Type::road);
        break;
    case Type::wall:
        edit_line.render(renderer, disp, gen_resources, Object::Type::wall);
        break;
    case Type::player:
        edit_basic.render(renderer, gen_resources, Object::Type::player);
        break;
    case Type::tree:
        edit_basic.render(renderer, gen_resources, Object::Type::tree);
        break;
    }
}
