#include "debug_panel.hpp"

void show_debug_panel(UX &ux, RuntimeMode &mode)
{
    ux.background_color(0x000000FF)
        .border_color(0x666666FF)
        .border_size(3)
        .margin(10)
        .padding(10);


    ux.column([&]() {
        ux.color(0xEEEEFFFF).heading("Debug panel");

        ux.color(0x999999FF).label("Select mode");

        ux.row([&]() {
            if (ux.button("Menu"))
            {
                mode = RuntimeMode::menu;
            }
            if (ux.button("Editor"))
            {
                mode = RuntimeMode::editor;
            }
            if (ux.button("Game"))
            {
                mode = RuntimeMode::game;
            }
        });
    });
}
