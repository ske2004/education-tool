#pragma once

#include "catedu/gui/editor/dispatcher.hpp"
#include "catedu/rendering/3d/pbr.hpp"
#include "catedu/rendering/resource_spec.hpp"
#include "catedu/sys/sokol_setup.hpp"
#include "sokol/sokol_gfx.h"

struct ViewportServer : SokolSetup
{
    Renderer renderer;
    ResourceSpec res;
    Camera camera;
    Dispatcher world;

    // Offscreen FBO
    sg_image color_img;
    sg_image depth_img;
    sg_sampler sampler;
    sg_attachments attachment;
    sg_pass_action offscreen_pa;

    // Frame dimensions
    int width;
    int height;

    // Pixel readback buffer
    uint8_t *pixels = nullptr;
    uint8_t *flip_row = nullptr;

    // Original stdout fd (saved before redirect)
    int frame_fd = -1;

    // stdin read buffer
    std::string stdin_buf;

    // Quit flag
    bool should_quit = false;

    ViewportServer(int w, int h);

    void init() override;
    void frame() override;
    void cleanup() override;
    void input(const sapp_event *event) override;

    void create_offscreen(int w, int h);
    void destroy_offscreen();
    void poll_stdin();
    void handle_command(const char *json_line);
    void send_frame();
};
