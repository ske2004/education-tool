#pragma once
#include "catedu/genobj/render.hpp"
#include "world.hpp"

struct GltfExportOptions
{
    bool single_scene_interiors = false;
};

// Writes the world as a binary glTF file (.glb) for other tools and engines.
// The outdoor place is the default scene and every interior gets a scene of
// its own. Each object is a node whose extras hold its editor properties.
bool export_world_gltf(const char *path, World &world,
                       const GenResources &resources);

bool export_world_gltf(const char *path, World &world,
                       const GenResources &resources,
                       GltfExportOptions options);
