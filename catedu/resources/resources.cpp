#include "resources.hpp"

struct ModelProto
{
    const char *name;
    const char *path;
    int submodel;
};

struct TileProto
{
    const char *name;
    const char *model_name;
    bool if_obstacle;
    int rotation;
};

const ModelProto model_protos[] = {
    {"invalid", "./assets/models/cube.gltf", 1},
    {"cube", "./assets/models/cube.gltf"},
    {"cube_shaded", "./assets/models/cube_shaded.gltf"},
    {"cube_window", "./assets/models/cube_window.gltf"},
    {"selector", "./assets/models/cube.gltf", 2},
    {"hitbox", "./assets/models/cube.gltf", 3},
    {"barrel", "./assets/models/barrel.gltf"},
    {"car", "./assets/models/car.gltf"},
    {"counter", "./assets/models/counter.gltf"},
    {"tile", "./assets/models/tile.gltf"},
    {"pavement", "./assets/models/tile.gltf", 1},
    {"woodtile", "./assets/models/tile.gltf", 2},
    {"wall", "./assets/models/wall_edge.gltf"},
    {"wall_wood", "./assets/models/wall_edge.gltf", 1},
    {"crate", "./assets/models/crate.gltf"},
    {"skybox", "./assets/models/skybox.gltf"},
    {"grass_floor", "./assets/models/floor.gltf"},
    {"player", "./assets/models/actor.gltf"},
    {"npc", "./assets/models/actor.gltf", 1},
    {"tree", "./assets/models/tree.gltf"}};

const TileProto tile_protos[] = {{"barrel", "barrel", true},
                                 {"counter", "counter", true},
                                 {"tile", "tile", false},
                                 {"wall_west", "wall", true, 2},
                                 {"wall_north", "wall", true, 1},
                                 {"wall_east", "wall", true, 0},
                                 {"wall_south", "wall", true, 3},
                                 {"crate", "crate", true},
                                 {"pavement", "pavement", false},
                                 {"wall_wood_west", "wall_wood", true, 2},
                                 {"wall_wood_north", "wall_wood", true, 1},
                                 {"wall_wood_east", "wall_wood", true, 0},
                                 {"wall_wood_south", "wall_wood", true, 3},
                                 {"woodtile", "woodtile", false},
                                 {"tree", "tree", true}};

ResourceSpec load_resource_spec(const char *path)
{
    ResourceSpec result = {};

    result.tileset = Texture::init("./assets/test_spritesheet_01.png");
    for (auto &proto : model_protos)
    {
        RawModel raw_model;
        bool ok = RawModel::load_gltf(proto.path, raw_model);
        assert(ok && "Failed to load model");
        Model model;
        ok = Model::load_from_raw(raw_model, model, proto.submodel);
        assert(ok && "Failed to create model");
        raw_model.deinit();
        result.models.allocate({proto.name, model, false});
    }

    for (auto &proto : tile_protos)
    {
        TableId model_id = result.find_model_by_name(proto.model_name);
        result.tiles.allocate(
            {proto.name, model_id, proto.if_obstacle, proto.rotation});
    }

    return result;
}
