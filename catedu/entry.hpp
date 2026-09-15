///
/// BRIEF: Entry point for everything
///

#pragma once
#include "catedu/gui/editor/editor.hpp"
#include "catedu/gui/main_menu/main_menu.hpp"
#include "catedu/gui/transition/transition.hpp"
#include "catedu/rendering/resource_spec.hpp"
#include "catedu/sys/fps.hpp"
#include "catedu/sys/input.hpp"
#include "catedu/sys/sokol_setup.hpp"
#include "catedu/umka/umka.hpp"
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"
#include <umka_api.h>

#include "catedu/runtime_mode.hpp"

struct Entry : SokolSetup
{
    UiState ui_state;

    RuntimeMode mode = RuntimeMode::menu;

    GuiMainMenu main_menu;
    GuiEditor editor;
    GuiTransition transition;
    ResourceSpec res;
    Dispatcher panorama;

    UmkaBindingsData umka_bindings_data;
    UmkaModule umka_module;

    bool show_debug;

    Renderer renderer;
    FpsCounter fps;

    void init();
    void frame();
    void cleanup();
    void input(const sapp_event *event);
};
