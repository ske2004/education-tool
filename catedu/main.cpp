#include "entry.hpp"
#include "viewport_server.hpp"
#include <cstring>

sapp_desc sokol_main(int argc, char *argv[])
{
    bool viewport_server = false;
    int width = 400;
    int height = 300;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--viewport-server") == 0)
        {
            viewport_server = true;
        }
        else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc)
        {
            width = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc)
        {
            height = atoi(argv[++i]);
        }
    }

    if (viewport_server)
    {
        sapp_desc desc =
            sokol_setup_default("Viewport Server", new ViewportServer(width, height));
        // Minimal window for GL context only
        desc.width = 1;
        desc.height = 1;
        return desc;
    }

    return sokol_setup_default("Education tool", new Entry());
}
