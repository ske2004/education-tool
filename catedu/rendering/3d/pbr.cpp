#include "pbr.hpp"
#include "catedu/shaders.hxx"
#include "sokol/sokol_app.h"
#include <cstring>
#include <sokol/sokol_glue.h>
#include <stdio.h>

static void init_pipelines(sg_shader shader, sg_pipeline &pipeline,
                           sg_pipeline &offscreen_pipeline)
{
    sg_pipeline_desc desc = {};

    desc.layout.buffers[0].stride = sizeof(float) * 8;
    // Vertex data
    desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3; // position
    desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT3; // normal
    desc.layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT2; // UV

    desc.colors[0].blend.enabled = true;
    desc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
    desc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
    desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ZERO;
    desc.shader = shader;
    desc.index_type = SG_INDEXTYPE_UINT16;
    desc.cull_mode = SG_CULLMODE_BACK;

    desc.depth.write_enabled = true;
    desc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;

    desc.label = "pbr-pipeline";

    sg_pipeline_desc offscreen_desc = desc;
    offscreen_desc.label = "pbr-offscreen-pipeline";
    offscreen_desc.colors[0].pixel_format = SG_PIXELFORMAT_RGBA8;
    offscreen_desc.depth.pixel_format = SG_PIXELFORMAT_DEPTH;

    pipeline = sg_make_pipeline(desc);
    offscreen_pipeline = sg_make_pipeline(offscreen_desc);
}

static sg_pass_action init_pass_action()
{
    sg_pass_action result = {};

    result.colors[0].load_action = SG_LOADACTION_CLEAR;
    result.colors[0].store_action = SG_STOREACTION_STORE;
    result.colors[0].clear_value = {0.2f, 0.2f, 0.7f, 1.0f};

    return result;
}

Renderer Renderer::init()
{
    Renderer result = {};

    result.pass_action = init_pass_action();
    result.shader = sg_make_shader(pbr_prog_shader_desc(sg_query_backend()));
    init_pipelines(result.shader, result.pipeline, result.offscreen_pipeline);

    return result;
}

void Renderer::deinit()
{
    sg_destroy_pipeline(offscreen_pipeline);
    sg_destroy_pipeline(pipeline);
    sg_destroy_shader(shader);
}

void Renderer::begin_pass()
{
    if (camera.aspect < 0.01f)
    {
        fprintf(stderr, "WARNING: Camera aspect ratio is too small, clamping\n");
        camera.aspect = 0.01f;
    }

    sg_pass pass = {0};
    pass.action = this->pass_action;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);

    sg_apply_pipeline(pipeline);
}

void Renderer::begin_pass_offscreen(sg_pass_action pa, sg_attachments att)
{
    if (camera.aspect < 0.01f)
    {
        fprintf(stderr, "WARNING: Camera aspect ratio is too small, clamping\n");
        camera.aspect = 0.01f;
    }

    sg_pass pass = {0};
    pass.action = pa;
    pass.attachments = att;
    sg_begin_pass(&pass);

    sg_apply_pipeline(offscreen_pipeline);
}

void Renderer::render_model(const Model &model, Params params)
{
    sg_bindings bindings = {};
    bindings.vertex_buffers[0] = model.vertex_buffer;
    bindings.index_buffer = model.index_buffer;
    bindings.fs.images[0] = model.texture.sysid_texture;
    bindings.fs.samplers[0] = model.texture.sysid_sampler;

    pbr_vs_params_t vs_params = {};
    memcpy(&vs_params, &params, sizeof(Params));

    sg_apply_bindings(bindings);

    vs_params.viewproj = this->camera.vp;

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_pbr_vs_params,
                      SG_RANGE(vs_params));

    sg_draw(0, model.index_count, 1);
}

void Renderer::end_pass()
{
    sg_end_pass();
}
