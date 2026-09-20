#ifndef __APPLE__
#define SOKOL_IMPL

#ifdef _WIN32
#define SOKOL_WIN32_FORCE_MAIN
#define SOKOL_D3D11
#elif defined(__EMSCRIPTEN__)
#define SOKOL_GLES3
#else
#define SOKOL_GLCORE
#endif

#else
#define SOKOL_METAL
#endif

#ifdef RUN_TESTS
#define SOKOL_NO_ENTRY
#endif

#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"

#include "imgui.h"
#define SOKOL_IMGUI_IMPL
#include "sokol/util/sokol_imgui.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include "lib/stb/stb_image.h"
#include "lib/stb/stb_truetype.h"

#define CGLTF_IMPLEMENTATION
#include "lib/cgltf/cgltf.h"
