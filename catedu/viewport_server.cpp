#include "viewport_server.hpp"
#include "catedu/resources/resources.hpp"
#include "catedu/scene/render_world.hpp"
#include "catedu/scene/world_file.hpp"
#include "catedu/sys/sg_tricks.hpp"
#include "sokol/sokol_glue.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

// Include OpenGL headers for glReadPixels + framebuffer functions
#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#elif defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#endif

// Minimal JSON parser helpers (no dependency needed for this simple protocol)
static bool json_match_str(const char *json, const char *key, char *out,
                           int out_size)
{
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p)
        return false;
    p += strlen(pattern);
    while (*p == ' ' || *p == ':' || *p == '\t')
        p++;
    if (*p == '"')
    {
        p++;
        int i = 0;
        while (*p && *p != '"' && i < out_size - 1)
        {
            out[i++] = *p++;
        }
        out[i] = '\0';
        return true;
    }
    return false;
}

static bool json_match_double(const char *json, const char *key, double *out)
{
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p)
        return false;
    p += strlen(pattern);
    while (*p == ' ' || *p == ':' || *p == '\t')
        p++;
    char *end;
    *out = strtod(p, &end);
    return end != p;
}

static bool json_match_bool(const char *json, const char *key, bool *out)
{
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p)
        return false;
    p += strlen(pattern);
    while (*p == ' ' || *p == ':' || *p == '\t')
        p++;
    if (strncmp(p, "true", 4) == 0)
    {
        *out = true;
        return true;
    }
    if (strncmp(p, "false", 5) == 0)
    {
        *out = false;
        return true;
    }
    return false;
}

ViewportServer::ViewportServer(int w, int h) : width(w), height(h)
{
}

void ViewportServer::init()
{
    // Save original stdout fd for binary frame output, then redirect
    // stdout to stderr so that any printf() from game code goes to stderr.
#ifndef _WIN32
    frame_fd = dup(STDOUT_FILENO);
    dup2(STDERR_FILENO, STDOUT_FILENO);

    // Set stdin to non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
#else
    frame_fd = _dup(_fileno(stdout));
    _dup2(_fileno(stderr), _fileno(stdout));
#endif

    sg_tricks_init();
    res = load_resource_spec("./assets/tileset.png");
    renderer = Renderer::init();
    world = WorldFile::load("assets/world.dat");

    // Set up camera: 45-degree pitch looking at origin, zoomed out
    camera = Camera::init(60.0f);
    camera.position = {0.0f, 8.0f, 8.0f};
    camera.pitch = 45.0f;
    camera.yaw = 0.0f;
    camera.set_aspect((float)width / (float)height);

    create_offscreen(width, height);
}

void ViewportServer::create_offscreen(int w, int h)
{
    width = w;
    height = h;

    sg_image_desc img_desc = {};
    img_desc.render_target = true;
    img_desc.width = w;
    img_desc.height = h;
    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;

    color_img = sg_make_image(&img_desc);

    img_desc.pixel_format = SG_PIXELFORMAT_DEPTH;
    depth_img = sg_make_image(&img_desc);

    sg_sampler_desc sampler_desc = {};
    sampler_desc.min_filter = SG_FILTER_LINEAR;
    sampler_desc.mag_filter = SG_FILTER_LINEAR;
    sampler = sg_make_sampler(&sampler_desc);

    sg_attachments_desc att_desc = {};
    att_desc.colors[0].image = color_img;
    att_desc.depth_stencil.image = depth_img;
    attachment = sg_make_attachments(att_desc);

    offscreen_pa = {};
    offscreen_pa.colors[0].load_action = SG_LOADACTION_CLEAR;
    offscreen_pa.colors[0].store_action = SG_STOREACTION_STORE;
    offscreen_pa.colors[0].clear_value = {0.2f, 0.2f, 0.7f, 1.0f};

    // Allocate pixel buffers
    if (pixels) ALLOCATOR_MALLOC.free(pixels);
    if (flip_row) ALLOCATOR_MALLOC.free(flip_row);
    pixels = (uint8_t *)ALLOCATOR_MALLOC.alloc(w * h * 4);
    flip_row = (uint8_t *)ALLOCATOR_MALLOC.alloc(w * 4);
}

void ViewportServer::destroy_offscreen()
{
    sg_destroy_attachments(attachment);
    sg_destroy_image(color_img);
    sg_destroy_image(depth_img);
    sg_destroy_sampler(sampler);
}

void ViewportServer::frame()
{
    poll_stdin();

    if (should_quit)
    {
        sapp_request_quit();
        return;
    }

    // Update camera aspect ratio
    camera.set_aspect((float)width / (float)height);
    renderer.camera = camera;

    // Render scene to offscreen FBO
    renderer.begin_pass_offscreen(offscreen_pa, attachment);

    // Render the current place in the world
    if (world.world.current)
    {
        render_place(*world.world.current, renderer, res, 12.0f);
    }

    renderer.end_pass();

    // Also do a dummy pass on the swapchain so sokol doesn't complain
    {
        sg_pass pass = {};
        pass.swapchain = sglue_swapchain();
        pass.action.colors[0].load_action = SG_LOADACTION_CLEAR;
        pass.action.colors[0].clear_value = {0, 0, 0, 1};
        sg_begin_pass(&pass);
        sg_end_pass();
    }

    send_frame();
}

void ViewportServer::send_frame()
{
    // Read pixels from offscreen FBO via GL
    sg_gl_image_info gl_info = sg_gl_query_image_info(color_img);
    unsigned int tex = gl_info.tex[0];

    unsigned int fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           tex, 0);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);

    // Flip vertically (OpenGL reads bottom-up)
    int row_bytes = width * 4;
    for (int y = 0; y < height / 2; y++)
    {
        uint8_t *top = pixels + y * row_bytes;
        uint8_t *bot = pixels + (height - 1 - y) * row_bytes;
        memcpy(flip_row, top, row_bytes);
        memcpy(top, bot, row_bytes);
        memcpy(bot, flip_row, row_bytes);
    }

    // Write binary frame: [u32 width][u32 height][u32 size][pixels]
    uint32_t header[3];
    header[0] = (uint32_t)width;
    header[1] = (uint32_t)height;
    header[2] = (uint32_t)(width * height * 4);

#ifndef _WIN32
    write(frame_fd, header, sizeof(header));
    write(frame_fd, pixels, header[2]);
#else
    _write(frame_fd, header, sizeof(header));
    _write(frame_fd, pixels, header[2]);
#endif
}

void ViewportServer::poll_stdin()
{
    char buf[4096];
#ifndef _WIN32
    ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n > 0)
    {
        stdin_buf.append(buf, n);
    }
#else
    int n = _read(_fileno(stdin), buf, sizeof(buf));
    if (n > 0)
    {
        stdin_buf.append(buf, n);
    }
#endif

    // Process complete lines
    size_t pos;
    while ((pos = stdin_buf.find('\n')) != std::string::npos)
    {
        std::string line = stdin_buf.substr(0, pos);
        stdin_buf.erase(0, pos + 1);

        // Strip \r if present
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (!line.empty())
        {
            handle_command(line.c_str());
        }
    }
}

void ViewportServer::handle_command(const char *json_line)
{
    char cmd[64] = {};
    if (!json_match_str(json_line, "cmd", cmd, sizeof(cmd)))
    {
        return;
    }

    if (strcmp(cmd, "resize") == 0)
    {
        double w = 0, h = 0;
        json_match_double(json_line, "w", &w);
        json_match_double(json_line, "h", &h);
        if (w > 0 && h > 0 && ((int)w != width || (int)h != height))
        {
            destroy_offscreen();
            create_offscreen((int)w, (int)h);
        }
    }
    else if (strcmp(cmd, "mouse_motion") == 0)
    {
        double dx = 0, dy = 0;
        bool middle = false, right = false;
        json_match_double(json_line, "dx", &dx);
        json_match_double(json_line, "dy", &dy);
        json_match_bool(json_line, "middle", &middle);
        json_match_bool(json_line, "right", &right);

        if (middle && right)
        {
            // Orbit: rotate camera around origin
            camera.rotate((float)dx * 0.3f, (float)dy * 0.3f);
        }
        else if (middle)
        {
            // Pan: move camera laterally
            camera.move((float)-dx * 0.02f, (float)dy * 0.02f, 0);
        }
    }
    else if (strcmp(cmd, "mouse_wheel") == 0)
    {
        double delta = 0;
        json_match_double(json_line, "delta", &delta);
        // Zoom: move camera forward/backward
        camera.move(0, 0, (float)delta * 0.5f);
    }
    else if (strcmp(cmd, "quit") == 0)
    {
        should_quit = true;
    }
}

void ViewportServer::cleanup()
{
    destroy_offscreen();
    ALLOCATOR_MALLOC.free(pixels);
    ALLOCATOR_MALLOC.free(flip_row);
    pixels = nullptr;
    flip_row = nullptr;

    res.deinit();
    renderer.deinit();
    sg_tricks_deinit();
    world.destroy();

#ifndef _WIN32
    if (frame_fd >= 0)
        close(frame_fd);
#else
    if (frame_fd >= 0)
        _close(frame_fd);
#endif
}

void ViewportServer::input(const sapp_event *event)
{
    // Minimal window — no input handling needed
    (void)event;
}
