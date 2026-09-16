///
/// @file pbr.hpp
/// @brief Declarations for the PBR (Physically Based Rendering) module.
///

#pragma once

// The include order matters here.
// clang-format off
#include "sokol/sokol_gfx.h"
#include "catedu/core/math/math.hpp"
// clang-format on

#include "catedu/rendering/3d/camera.hpp"
#include "model.hpp"

struct Params
{
    Vector4 color_mul;
    Matrix4 model;
    Matrix4 viewproj;
    float lightness;
};

struct Renderer
{
    sg_shader shader;
    sg_pipeline pipeline;
    sg_pipeline offscreen_pipeline;
    sg_pass_action pass_action;
    Camera camera;

    static Renderer init();
    void deinit();

    void begin_pass();
    void begin_pass_offscreen(sg_pass_action pa, sg_attachments att);
    void render_model(const Model &model, Params params);
    void end_pass();
};
