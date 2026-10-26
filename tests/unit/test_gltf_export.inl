#include "catedu/resources/resources.hpp"
#include "catedu/rendering/resource_spec.hpp"
#include "catedu/scene/gltf_export.hpp"
#include "cgltf/cgltf.h"
#include "tau/tau.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static const char *const GLTF_TEST_PATH = "bin/gltf_export_test.glb";

// Exports the world and reads the file back. The exporter only needs the
// names of the models, so no GPU is needed.
static cgltf_data *export_and_parse(World &world,
                                    GltfExportOptions export_options = {})
{
    ResourceSpec spec = load_resource_names();
    GenResources resources = {};
    resources.spec = &spec;

    bool exported =
        export_world_gltf(GLTF_TEST_PATH, world, resources, export_options);
    spec.models.deinit();
    if (!exported)
    {
        return nullptr;
    }

    cgltf_options options = {};
    cgltf_data *data = nullptr;
    if (cgltf_parse_file(&options, GLTF_TEST_PATH, &data) !=
            cgltf_result_success ||
        cgltf_load_buffers(&options, data, GLTF_TEST_PATH) !=
            cgltf_result_success ||
        cgltf_validate(data) != cgltf_result_success)
    {
        cgltf_free(data);
        data = nullptr;
    }

    remove(GLTF_TEST_PATH);
    return data;
}

static cgltf_node *find_node(cgltf_data *data, const char *name)
{
    for (size_t i = 0; i < data->nodes_count; i++)
    {
        if (data->nodes[i].name && strcmp(data->nodes[i].name, name) == 0)
        {
            return &data->nodes[i];
        }
    }

    return nullptr;
}

static Object make_object(Object::Type type, float x, float y)
{
    Object object = {};
    object.type = type;
    object.floors = 1;
    object.x = x;
    object.y = y;
    return object;
}

static Object make_prop(const char *id, float x, float y)
{
    Object object = make_object(Object::Type::prop, x, y);
    strcpy(object.id, id);
    return object;
}

TEST(GltfExport, WritesEveryObject)
{
    World world = World::create();
    Object building = make_object(Object::Type::building, 0, 0);
    building.floors = 3;
    REQUIRE_TRUE(world.first->place_object(building));
    REQUIRE_TRUE(
        world.first->place_object(make_object(Object::Type::road, 10, 0)));
    REQUIRE_TRUE(world.first->place_object(make_prop("barrel", 12, 0)));
    REQUIRE_TRUE(world.first->place_object(make_prop("bench", 14, 0)));
    REQUIRE_TRUE(world.first->place_object(make_prop("no_such_prop", 16, 0)));

    cgltf_data *data = export_and_parse(world);
    REQUIRE_TRUE(data);

    CHECK_EQ(data->scenes_count, (size_t)1);
    // The ground and the five objects.
    CHECK_EQ(data->scene->nodes_count, (size_t)6);
    CHECK_TRUE(find_node(data, "barrel"));
    CHECK_TRUE(find_node(data, "bench"));
    CHECK_TRUE(find_node(data, "cube_window"));
    CHECK_TRUE(find_node(data, "grass_floor"));
    CHECK_TRUE(find_node(data, "invalid"));
    CHECK_TRUE(data->images_count > 0);

    cgltf_node *barrel = find_node(data, "barrel");
    REQUIRE_TRUE(barrel && barrel->extras.data);
    CHECK_TRUE(strstr(barrel->extras.data, "\"id\":\"barrel\""));

    cgltf_free(data);
    world.destroy();
}

TEST(GltfExport, InteriorsGetTheirOwnScene)
{
    World world = World::create();
    Object *building =
        world.first->place_object(make_object(Object::Type::building, 0, 0));
    REQUIRE_TRUE(building);
    building->place = world.places.alloc();
    *building->place = Place::create();
    building->place->interior = true;
    REQUIRE_TRUE(
        building->place->place_object(make_object(Object::Type::wall, 2, 2)));

    cgltf_data *data = export_and_parse(world);
    REQUIRE_TRUE(data);

    REQUIRE_EQ(data->scenes_count, (size_t)2);
    CHECK_EQ(data->scene, &data->scenes[0]);
    // The ground and the wall.
    CHECK_EQ(data->scenes[1].nodes_count, (size_t)2);

    cgltf_node *node = find_node(data, "building");
    REQUIRE_TRUE(node && node->extras.data);
    CHECK_TRUE(strstr(node->extras.data, "\"interior_scene\":1"));

    cgltf_free(data);
    world.destroy();
}

// The editor is left-handed and glTF is right-handed, so the Z axis flips.
TEST(GltfExport, MirrorsTheZAxis)
{
    World world = World::create();
    REQUIRE_TRUE(
        world.first->place_object(make_object(Object::Type::tree, 2, 5)));

    cgltf_data *data = export_and_parse(world);
    REQUIRE_TRUE(data);

    cgltf_node *tree = find_node(data, "tree");
    REQUIRE_TRUE(tree && tree->has_matrix);
    CHECK_EQ(tree->matrix[12], 2.0f);
    CHECK_EQ(tree->matrix[14], -5.0f);

    cgltf_free(data);
    world.destroy();
}

// Mirroring the geometry turns triangles inside out unless their winding is
// flipped too, so every triangle should still face along its normals.
TEST(GltfExport, TrianglesFaceOutwards)
{
    World world = World::create();
    REQUIRE_TRUE(
        world.first->place_object(make_object(Object::Type::road, 0, 0)));

    cgltf_data *data = export_and_parse(world);
    REQUIRE_TRUE(data);

    cgltf_node *cube = find_node(data, "cube");
    REQUIRE_TRUE(cube && cube->mesh);
    const cgltf_primitive &primitive = cube->mesh->primitives[0];
    const cgltf_accessor *positions = nullptr;
    const cgltf_accessor *normals = nullptr;
    for (size_t i = 0; i < primitive.attributes_count; i++)
    {
        if (primitive.attributes[i].type == cgltf_attribute_type_position)
        {
            positions = primitive.attributes[i].data;
        }
        if (primitive.attributes[i].type == cgltf_attribute_type_normal)
        {
            normals = primitive.attributes[i].data;
        }
    }
    REQUIRE_TRUE(positions && normals && primitive.indices);

    size_t facing = 0;
    size_t triangles = primitive.indices->count / 3;
    for (size_t i = 0; i < triangles; i++)
    {
        float p[3][3], n[3];
        for (int j = 0; j < 3; j++)
        {
            size_t index =
                cgltf_accessor_read_index(primitive.indices, i * 3 + j);
            cgltf_accessor_read_float(positions, index, p[j], 3);
            if (j == 0)
            {
                cgltf_accessor_read_float(normals, index, n, 3);
            }
        }

        float a[3], b[3];
        for (int k = 0; k < 3; k++)
        {
            a[k] = p[1][k] - p[0][k];
            b[k] = p[2][k] - p[0][k];
        }
        float cross[3] = {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
                          a[0] * b[1] - a[1] * b[0]};
        if (cross[0] * n[0] + cross[1] * n[1] + cross[2] * n[2] > 0)
        {
            facing++;
        }
    }
    CHECK_TRUE(triangles > 0);
    CHECK_EQ(facing, triangles);

    cgltf_free(data);
    world.destroy();
}

TEST(GltfExport, ExportsTimeOfDayAndScriptExtras)
{
    World world = World::create();
    world.time_of_day = 14.5f;

    ScriptNode *start_ev = world.script->acquire_start_event();
    REQUIRE_TRUE(start_ev != nullptr);
    ScriptNode *say =
        world.script->append_node(ScriptNode::Type::say, start_ev);
    strcpy(say->say, "Hello GlTF!");

    cgltf_data *data = export_and_parse(world);
    REQUIRE_TRUE(data);

    REQUIRE_TRUE(data->scene != nullptr);
    REQUIRE_TRUE(data->scene->extras.data != nullptr);

    CHECK_TRUE(strstr(data->scene->extras.data, "\"time_of_day\":14.5"));
    CHECK_TRUE(strstr(data->scene->extras.data, "\"sun_light\":"));
    CHECK_TRUE(strstr(data->scene->extras.data, "\"direction\":"));
    CHECK_TRUE(strstr(data->scene->extras.data, "\"color\":"));
    CHECK_TRUE(strstr(data->scene->extras.data, "\"script\":"));
    CHECK_TRUE(strstr(data->scene->extras.data, "\"Hello GlTF!\""));

    // Verify punctual directional light is exported in glTF extensions
    CHECK_TRUE(data->lights_count > 0);
    if (data->lights_count > 0)
    {
        CHECK_EQ(data->lights[0].type, cgltf_light_type_directional);
        CHECK_TRUE(data->lights[0].name &&
                   strcmp(data->lights[0].name, "sun") == 0);
    }

    cgltf_free(data);
    world.destroy();
}

TEST(GltfExport, SingleSceneInteriorsOption)
{
    World world = World::create();
    Object *building =
        world.first->place_object(make_object(Object::Type::building, 0, 0));
    REQUIRE_TRUE(building);
    building->place = world.places.alloc();
    *building->place = Place::create();
    building->place->interior = true;
    REQUIRE_TRUE(
        building->place->place_object(make_object(Object::Type::wall, 2, 2)));

    GltfExportOptions options = {};
    options.single_scene_interiors = true;
    cgltf_data *data = export_and_parse(world, options);
    REQUIRE_TRUE(data);

    // Default scene includes interior root node
    REQUIRE_TRUE(data->scene != nullptr);
    CHECK_TRUE(data->scene->nodes_count >= 3);

    // Root node for the interior exists and is positioned offset
    cgltf_node *interior_node = find_node(data, "building interior (0, 0)");
    REQUIRE_TRUE(interior_node != nullptr);
    CHECK_TRUE(interior_node->has_matrix);
    CHECK_EQ(interior_node->matrix[12], 64.0f);
    CHECK_EQ(interior_node->matrix[13], -32.0f);

    // Interior object exists in node hierarchy
    cgltf_node *wall = find_node(data, "wall");
    CHECK_TRUE(wall != nullptr);

    // Building extras reference interior_node
    cgltf_node *building_node = find_node(data, "building");
    REQUIRE_TRUE(building_node && building_node->extras.data);
    CHECK_TRUE(strstr(building_node->extras.data, "\"interior_node\""));

    cgltf_free(data);
    world.destroy();
}
