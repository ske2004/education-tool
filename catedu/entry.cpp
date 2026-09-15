#include "entry.hpp"
#include "catedu/gui/debug_panel.hpp"
#include "catedu/gui/debugtree/debugtree.hpp"
#include "catedu/gui/transition/transition.hpp"
#include "catedu/resources/resources.hpp"
#include "catedu/scene/world_file.hpp"
#include "catedu/sys/input.hpp"
#include "catedu/sys/sg_tricks.hpp"
#include "catedu/ui/ux.hpp"

void Entry::frame(void)
{
    fps.update();

    bool reload_module = false;

    UX ux = UX::begin(ui_state);
    UiPass &pass = ux.pass;

    bool returned_to_menu = false;

    switch (mode)
    {
    case RuntimeMode::debug:
        show_debug_panel(ux, mode);
        if (mode == RuntimeMode::editor)
        {
            this->editor = GuiEditor::init(&this->ui_state);
        }
        break;
    case RuntimeMode::menu:
        if (this->main_menu.show(ux, transition, this->panorama.world,
                                 this->renderer, this->res))
        {
            this->editor = GuiEditor::init(&this->ui_state);
            this->mode = RuntimeMode::editor;
        }
        break;
    case RuntimeMode::editor:
    case RuntimeMode::game:
        if (editor.show(pass, transition, this->renderer, this->res))
        {
            mode = RuntimeMode::menu;
            returned_to_menu = true;
        }
        break;
    }

    transition.show(pass, pass.state->input);

    if (pass.state->input.k[SAPP_KEYCODE_F3].pressed)
    {
        pass.state->input.clear(SAPP_KEYCODE_F3);
        show_debug = !show_debug;
    }

    DEBUG_TREE.reset();
    DEBUG_TREE.value("frees", (uint64_t)ALLOCATOR_MALLOC.tracer.total_frees);
    DEBUG_TREE.value("allocs",
                     (uint64_t)ALLOCATOR_MALLOC.tracer.total_allocations);
    DEBUG_TREE.size("bytes",
                    (uint64_t)ALLOCATOR_MALLOC.tracer.total_bytes_allocated);
    DEBUG_TREE.value("fps", fps.get());

    if (show_debug)
    {
        DEBUG_TREE.show(pass);
    }

    ux.end();

    // FIXME: We need to defer `editor.deinit`, because `deinit` removes the
    // target images, which are still in use during the UI pass.
    if (returned_to_menu)
    {
        this->editor.deinit();
    }

    if (reload_module)
    {
        this->umka_module.reload();
    }
}

void Entry::cleanup(void)
{
    main_menu.deinit();
    if (mode == RuntimeMode::editor)
    {
        editor.deinit();
    }
    res.deinit();
    ui_state.deinit();
    this->renderer.deinit();
    sg_tricks_deinit();

    this->umka_module.destroy();

    this->panorama.destroy();

    DEBUG_TREE.deinit();

    // NOTE: Some memory will show as "leak" because this function is called
    // before the sokol deinitializers are called.
    // TODO: Trace the memory allocations before initialization and correctly
    // display them.
    fprintf(stderr, "Memory leaked: %zu\n",
            ALLOCATOR_MALLOC.tracer.total_bytes_allocated);
}

void Entry::init()
{
    this->umka_module = UmkaModule::create(&this->umka_bindings_data);

    sg_tricks_init();

    res = load_resource_spec("./assets/tileset.png");

    ui_state = UiState::init("./assets/Roboto-Regular.ttf",
                             "./assets/Roboto-Bold.ttf", sapp_dpi_scale());

    main_menu = GuiMainMenu::init(&ui_state);

    if (mode == RuntimeMode::editor)
    {
        editor = GuiEditor::init(&ui_state);
    }

    panorama = WorldFile::load("assets/world.dat");

    transition = GuiTransition::create();

    this->renderer = Renderer::init();
}

void Entry::input(const sapp_event *event)
{
    if (event->type == SAPP_EVENTTYPE_QUIT_REQUESTED)
    {
        if (this->editor.dispatcher.dirty)
        {
            sapp_cancel_quit();
            this->editor.exit_requested = true;
        }
    }

    if (event->type == SAPP_EVENTTYPE_UNFOCUSED)
    {
        this->ui_state.input.reset();
    }

    if (ui_state.feed_event(event))
    {
        return;
    }
}
