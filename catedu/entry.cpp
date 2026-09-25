#include "entry.hpp"
#include "catedu/gui/debug_panel.hpp"
#include "catedu/gui/debugtree/debugtree.hpp"
#include "catedu/gui/transition/transition.hpp"
#include "catedu/resources/resources.hpp"
#include "catedu/scene/world_file.hpp"
#include "catedu/sys/input.hpp"
#include "catedu/sys/sg_tricks.hpp"
#include "catedu/ui/ux.hpp"
#include <imgui.h>
#include <sokol/util/sokol_imgui.h>

void Entry::frame(void)
{
    simgui_new_frame({ sapp_width(), sapp_height(), sapp_frame_duration(), sapp_dpi_scale() });
    
    ImGui::Begin("EduArts Debug");
    ImGui::Text("Welcome to the ImGui Debug Panel!");
    if (ImGui::Button("Toggle Demo Window")) {
        show_debug = !show_debug;
    }
    if (show_debug) {
        ImGui::ShowDemoWindow(&show_debug);
    }
    
    static bool show_ai_paths = false;
    ImGui::Checkbox("Show AI Paths", &show_ai_paths);
    
    if (show_ai_paths && mode == RuntimeMode::editor && editor.playtesting) {
        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
        Matrix4 vp = editor.editor_camera.cam.vp;
        float width = sapp_widthf();
        float height = sapp_heightf();
        
        for (auto [id, npc] : iter(editor.playtest.npcs)) {
            if (npc.path_count == 0) continue;
            
            PhysicsBody &b = editor.playtest.physics.bodies.get_assert(npc.body);
            Vector2 pos = b.area.pos;
            
            Vector3 current_point = {pos.x, 0, pos.y};
            Vector4 proj = vp * Vector4{current_point.x, current_point.y, current_point.z, 1.0f};
            if (proj.w > 0.0f) {
                ImVec2 p1 = { (proj.x / proj.w * 0.5f + 0.5f) * width, (0.5f - proj.y / proj.w * 0.5f) * height };
                
                for (int i = npc.path_index; i < npc.path_count; i++) {
                    Vector3 next_point = {(float)npc.path[i].x, 0, (float)npc.path[i].y};
                    Vector4 next_proj = vp * Vector4{next_point.x, next_point.y, next_point.z, 1.0f};
                    if (next_proj.w > 0.0f) {
                        ImVec2 p2 = { (next_proj.x / next_proj.w * 0.5f + 0.5f) * width, (0.5f - next_proj.y / next_proj.w * 0.5f) * height };
                        draw_list->AddLine(p1, p2, IM_COL32(255, 50, 50, 255), 2.0f);
                        draw_list->AddCircleFilled(p2, 4.0f, IM_COL32(255, 200, 50, 255));
                        p1 = p2;
                    }
                }
            }
        }
    }
    ImGui::End();

    if (mode == RuntimeMode::editor && editor.playtesting) {
        ImGui::Begin("Simulation State");
        
        int hours = (int)editor.playtest.world.time_of_day;
        int mins = (int)((editor.playtest.world.time_of_day - hours) * 60);
        ImGui::Text("Time of Day: %02d:%02d", hours, mins);
        ImGui::Separator();
        
        ImGui::Text("Inventory (%zu / %d):", editor.playtest.world.player_inventory.items.size(), editor.playtest.world.player_inventory.capacity);
        
        if (editor.playtest.world.player_inventory.items.empty()) {
            ImGui::TextDisabled("Inventory is empty.");
        } else {
            if (ImGui::BeginTable("inventory_table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Item");
                ImGui::TableSetupColumn("Amount");
                ImGui::TableSetupColumn("Description");
                ImGui::TableHeadersRow();
                
                for (const auto& item : editor.playtest.world.player_inventory.items) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(item.name.c_str());
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", item.amount);
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(item.description.c_str());
                }
                ImGui::EndTable();
            }
        }
        ImGui::End();
    }
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

    sg_pass_action pass_action = {};
    pass_action.colors[0].load_action = SG_LOADACTION_LOAD;
    sg_pass imgui_pass = {};
    imgui_pass.action = pass_action;
    imgui_pass.swapchain = sglue_swapchain();
    sg_begin_pass(&imgui_pass);
    simgui_render();
    sg_end_pass();
}

void Entry::cleanup(void)
{
    simgui_shutdown();
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
    simgui_desc_t desc = {};
    simgui_setup(&desc);

    this->umka_module = UmkaModule::create(&this->umka_bindings_data);

    sg_tricks_init();

    res = load_resource_spec();

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
    simgui_handle_event(event);
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
