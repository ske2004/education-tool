#include "load_image.hpp"
#include "catedu/ui/rendering/core.hpp"
#include "lib/stb/stb_image.h"

UiImageId ui_resources_load_image(UiRenderingCore *core, const char *path)
{
    // Load image with STB
    int width, height, channels;
    stbi_uc *data = stbi_load(path, &width, &height, &channels, 4);
    assert(data);

    // Vertically flip image
    // (TODO: Look into why this is necessary)
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width / 2; x++)
        {
            for (int c = 0; c < 4; c++)
            {
                int i0 = (y * width + x) * 4 + c;
                int i1 = (y * width + width - x - 1) * 4 + c;
                stbi_uc tmp = data[i0];
                data[i0] = data[i1];
                data[i1] = tmp;
            }
        }
    }

    // Create image
    UiImageId id = ui_resources_load_image_from_memory(
        core, {data, (size_t)(width * height * 4)}, {width, height});
    stbi_image_free(data);
    return id;
}

UiImageId ui_resources_load_image_from_memory(UiRenderingCore *core,
                                              Buffer buffer, Vector2i size)
{
    return core->alloc_image(ui_image_make_from_data(buffer, size));
}
