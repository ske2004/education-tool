#include "gltf_export.hpp"
#include "catedu/resources/resources.hpp"
#include "catedu/sys/json_fields.hpp"
#include "render_world.hpp"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

// The editor's world is left-handed and glTF is right-handed, so everything is
// mirrored along the Z axis to keep the scene looking the way it does in the
// editor.

enum
{
    GLTF_FLOAT = 5126,
    GLTF_UNSIGNED_INT = 5125,
    GLTF_ARRAY_BUFFER = 34962,
    GLTF_ELEMENT_ARRAY_BUFFER = 34963,
    GLTF_LINEAR = 9729,
};

struct GltfGeometry
{
    std::string path;
    // Accessor indices, -1 when the source model doesn't have the attribute.
    int position, normal, uv, indices;
};

// A model from the resource spec, resolved to what the file stores for it.
struct GltfModel
{
    std::string name;
    int geometry;
    int image;
};

struct GltfMaterial
{
    int image;
    Vector4 color;
};

struct GltfMesh
{
    std::string name;
    int geometry;
    int material;
};

struct GltfNode
{
    std::string name;
    Matrix4 matrix;
    int mesh;
    std::vector<int> children;
    // A JSON object, or empty for none.
    std::string extras;
};

struct GltfScene
{
    std::string name;
    std::vector<int> nodes;
    std::string extras;
};

struct GltfExport
{
    const GenResources *resources;
    GltfExportOptions options;
    int interior_count;
    Vector3 sun_color;

    std::vector<uint8_t> bin;
    // The JSON of each buffer view and accessor, they're written as they are.
    std::vector<std::string> buffer_views;
    std::vector<std::string> accessors;

    std::vector<std::string> images;
    std::vector<int> image_views;
    std::vector<GltfGeometry> geometries;
    std::vector<GltfModel> models;
    std::vector<GltfMaterial> materials;
    std::vector<GltfMesh> meshes;
    std::vector<GltfNode> nodes;
    std::vector<GltfScene> scenes;
};

static void append_int_array(std::string &out, const std::vector<int> &values)
{
    out += '[';
    for (size_t i = 0; i < values.size(); i++)
    {
        appendf(out, i ? ",%d" : "%d", values[i]);
    }
    out += ']';
}

static bool is_identity(const Matrix4 &matrix)
{
    // Compared by value, mirroring the identity matrix leaves negative zeros.
    Matrix4 identity = Matrix4::identity();
    for (int i = 0; i < 16; i++)
    {
        if (matrix.values[i] != identity.values[i])
        {
            return false;
        }
    }

    return true;
}

static Matrix4 mirror_z(Matrix4 matrix)
{
    for (int column = 0; column < 4; column++)
    {
        for (int row = 0; row < 4; row++)
        {
            if ((column == 2) != (row == 2))
            {
                matrix.values[column * 4 + row] *= -1;
            }
        }
    }

    return matrix;
}

static float srgb_to_linear(float c)
{
    return c <= 0.04045f ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f);
}

static int add_buffer_view(GltfExport &e, const void *data, size_t size,
                           int target)
{
    while (e.bin.size() % 4)
    {
        e.bin.push_back(0);
    }

    size_t offset = e.bin.size();
    e.bin.insert(e.bin.end(), (const uint8_t *)data,
                 (const uint8_t *)data + size);

    std::string json;
    appendf(json, "{\"buffer\":0,\"byteOffset\":%zu,\"byteLength\":%zu", offset,
            size);
    if (target)
    {
        appendf(json, ",\"target\":%d", target);
    }
    json += '}';

    e.buffer_views.push_back(json);
    return (int)e.buffer_views.size() - 1;
}

static int add_accessor(GltfExport &e, const void *data, size_t count,
                        int component_type, const char *type, int components,
                        int target)
{
    int view = add_buffer_view(e, data, count * components * 4, target);

    std::string json;
    appendf(json,
            "{\"bufferView\":%d,\"componentType\":%d,\"count\":%zu,"
            "\"type\":\"%s\"",
            view, component_type, count, type);
    json += '}';

    e.accessors.push_back(json);
    return (int)e.accessors.size() - 1;
}

static int add_positions(GltfExport &e, const std::vector<float> &positions)
{
    size_t count = positions.size() / 3;
    float min[3] = {INFINITY, INFINITY, INFINITY};
    float max[3] = {-INFINITY, -INFINITY, -INFINITY};
    for (size_t i = 0; i < count; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            min[j] = fminf(min[j], positions[i * 3 + j]);
            max[j] = fmaxf(max[j], positions[i * 3 + j]);
        }
    }

    int accessor = add_accessor(e, positions.data(), count, GLTF_FLOAT, "VEC3",
                                3, GLTF_ARRAY_BUFFER);

    // Position accessors are required to have bounds.
    std::string &json = e.accessors[accessor];
    json.pop_back();
    appendf(json, ",\"min\":[%.9g,%.9g,%.9g],\"max\":[%.9g,%.9g,%.9g]}", min[0],
            min[1], min[2], max[0], max[1], max[2]);
    return accessor;
}

static const cgltf_accessor *find_attribute(const cgltf_primitive &primitive,
                                            cgltf_attribute_type type)
{
    for (size_t i = 0; i < primitive.attributes_count; i++)
    {
        if (primitive.attributes[i].type == type &&
            primitive.attributes[i].index == 0)
        {
            return primitive.attributes[i].data;
        }
    }

    return nullptr;
}

static std::vector<float> unpack_floats(const cgltf_accessor *accessor)
{
    std::vector<float> result(accessor->count *
                              cgltf_num_components(accessor->type));
    cgltf_accessor_unpack_floats(accessor, result.data(), result.size());
    return result;
}

static int add_geometry(GltfExport &e, const char *path, const RawModel &raw)
{
    for (size_t i = 0; i < e.geometries.size(); i++)
    {
        if (e.geometries[i].path == path)
        {
            return (int)i;
        }
    }

    // Like the renderer, only the first primitive of the first mesh is used.
    if (raw.data->meshes_count == 0 ||
        raw.data->meshes[0].primitives_count == 0)
    {
        fprintf(stderr, "glTF export: '%s' has no mesh\n", path);
        return -1;
    }

    const cgltf_primitive &primitive = raw.data->meshes[0].primitives[0];
    const cgltf_accessor *position =
        find_attribute(primitive, cgltf_attribute_type_position);
    const cgltf_accessor *normal =
        find_attribute(primitive, cgltf_attribute_type_normal);
    const cgltf_accessor *uv =
        find_attribute(primitive, cgltf_attribute_type_texcoord);

    if (primitive.type != cgltf_primitive_type_triangles || !position)
    {
        fprintf(stderr, "glTF export: '%s' has no triangles\n", path);
        return -1;
    }

    GltfGeometry geometry = {path, -1, -1, -1, -1};

    std::vector<float> positions = unpack_floats(position);
    for (size_t i = 2; i < positions.size(); i += 3)
    {
        positions[i] = -positions[i];
    }
    geometry.position = add_positions(e, positions);

    if (normal)
    {
        std::vector<float> normals = unpack_floats(normal);
        for (size_t i = 2; i < normals.size(); i += 3)
        {
            normals[i] = -normals[i];
        }
        geometry.normal =
            add_accessor(e, normals.data(), normal->count, GLTF_FLOAT, "VEC3",
                         3, GLTF_ARRAY_BUFFER);
    }

    if (uv)
    {
        std::vector<float> uvs = unpack_floats(uv);
        geometry.uv = add_accessor(e, uvs.data(), uv->count, GLTF_FLOAT, "VEC2",
                                   2, GLTF_ARRAY_BUFFER);
    }

    std::vector<uint32_t> indices;
    if (primitive.indices)
    {
        for (size_t i = 0; i < primitive.indices->count; i++)
        {
            indices.push_back(
                (uint32_t)cgltf_accessor_read_index(primitive.indices, i));
        }
    }
    else
    {
        for (size_t i = 0; i < position->count; i++)
        {
            indices.push_back((uint32_t)i);
        }
    }

    // Mirroring turns the triangles inside out, so flip their winding.
    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        uint32_t index = indices[i + 1];
        indices[i + 1] = indices[i + 2];
        indices[i + 2] = index;
    }
    geometry.indices =
        add_accessor(e, indices.data(), indices.size(), GLTF_UNSIGNED_INT,
                     "SCALAR", 1, GLTF_ELEMENT_ARRAY_BUFFER);

    e.geometries.push_back(geometry);
    return (int)e.geometries.size() - 1;
}

static int add_image(GltfExport &e, const char *path)
{
    for (size_t i = 0; i < e.images.size(); i++)
    {
        if (e.images[i] == path)
        {
            return (int)i;
        }
    }

    const char *extension = strrchr(path, '.');
    if (!extension || strcmp(extension, ".png") != 0)
    {
        fprintf(stderr, "glTF export: '%s' isn't a PNG, skipping it\n", path);
        return -1;
    }

    FILE *f = fopen(path, "rb");
    if (!f)
    {
        fprintf(stderr, "glTF export: failed to open '%s'\n", path);
        return -1;
    }

    std::vector<uint8_t> data;
    uint8_t chunk[4096];
    size_t read;
    while ((read = fread(chunk, 1, sizeof chunk, f)) > 0)
    {
        data.insert(data.end(), chunk, chunk + read);
    }
    fclose(f);

    e.images.push_back(path);
    e.image_views.push_back(add_buffer_view(e, data.data(), data.size(), 0));
    return (int)e.images.size() - 1;
}

static const GltfModel *find_model(GltfExport &e, const char *name)
{
    for (const GltfModel &model : e.models)
    {
        if (model.name == name)
        {
            return &model;
        }
    }

    GltfModel model = {name, -1, -1};

    const char *path;
    int submodel;
    RawModel raw;
    if (!find_model_source(name, &path, &submodel))
    {
        fprintf(stderr, "glTF export: no source for model '%s'\n", name);
    }
    else if (RawModel::load_gltf(path, raw))
    {
        model.geometry = add_geometry(e, path, raw);

        char texture_path[1024];
        if (raw.submodel_texture_path(submodel, texture_path,
                                      sizeof texture_path))
        {
            model.image = add_image(e, texture_path);
        }

        raw.deinit();
    }

    e.models.push_back(model);
    return &e.models.back();
}

static int add_material(GltfExport &e, int image, Vector4 color)
{
    for (size_t i = 0; i < e.materials.size(); i++)
    {
        const GltfMaterial &material = e.materials[i];
        if (material.image == image && material.color.x == color.x &&
            material.color.y == color.y && material.color.z == color.z &&
            material.color.w == color.w)
        {
            return (int)i;
        }
    }

    e.materials.push_back({image, color});
    return (int)e.materials.size() - 1;
}

static int add_mesh(GltfExport &e, const char *name, int geometry, int material)
{
    for (size_t i = 0; i < e.meshes.size(); i++)
    {
        if (e.meshes[i].geometry == geometry &&
            e.meshes[i].material == material)
        {
            return (int)i;
        }
    }

    e.meshes.push_back({name, geometry, material});
    return (int)e.meshes.size() - 1;
}

static int add_node(GltfExport &e, const char *name, Matrix4 matrix,
                    int mesh = -1)
{
    e.nodes.push_back({name, mirror_z(matrix), mesh, {}, {}});
    return (int)e.nodes.size() - 1;
}

struct NodeSink
{
    GltfExport *e;
    int parent;
};

static void add_draw(void *user, const GenDraw &draw)
{
    NodeSink *sink = (NodeSink *)user;
    GltfExport &e = *sink->e;

    const GltfModel *model = find_model(e, draw.model_name);
    if (model->geometry < 0)
    {
        return;
    }

    int material = add_material(e, model->image, draw.color);
    int mesh = add_mesh(e, draw.model_name, model->geometry, material);
    int node = add_node(e, draw.model_name, draw.matrix, mesh);
    e.nodes[sink->parent].children.push_back(node);
}

static int export_interior_node(GltfExport &e, Place &place, const char *name,
                                int interior_index)
{
    Matrix4 offset =
        Matrix4::translate({interior_index * 64.0f, -32.0f, 0.0f});
    int root = add_node(e, name, offset);

    int ground = add_node(e, "ground", Matrix4::identity());
    e.nodes[root].children.push_back(ground);
    NodeSink ground_sink = {&e, ground};
    emit_place_ground({add_draw, &ground_sink}, *e.resources, place);

    for (auto &object : iter(place.objects))
    {
        const char *type_name = object_type_name(object.type);
        bool named_prop = object.type == Object::Type::prop && object.id[0];

        int node = add_node(e, named_prop ? object.id : type_name,
                            place_object_matrix(object));
        e.nodes[root].children.push_back(node);

        NodeSink sink = {&e, node};
        emit_place_object({add_draw, &sink}, *e.resources, object,
                          Matrix4::identity());

        std::string extras = "{\"type\":";
        append_json_string(extras, type_name);
        appendf(extras, ",\"x\":%.9g,\"y\":%.9g,\"floors\":%d", object.x,
                object.y, object.floors);
        if (object.type == Object::Type::prop)
        {
            extras += ",\"id\":";
            append_json_string(extras, object.id);
        }
        if (object.place)
        {
            char nested_interior_name[128];
            snprintf(nested_interior_name, sizeof nested_interior_name,
                     "%s interior (%g, %g)", type_name, object.x, object.y);
            e.interior_count++;
            int nested_root = export_interior_node(
                e, *object.place, nested_interior_name, e.interior_count);
            e.scenes[0].nodes.push_back(nested_root);
            appendf(extras, ",\"interior_node\":%d", nested_root);
        }
        extras += '}';
        e.nodes[node].extras = extras;
    }

    return root;
}

static int export_place(GltfExport &e, Place &place, const char *name)
{
    int scene = (int)e.scenes.size();
    e.scenes.push_back({name, {}});

    int ground = add_node(e, "ground", Matrix4::identity());
    e.scenes[scene].nodes.push_back(ground);
    NodeSink ground_sink = {&e, ground};
    emit_place_ground({add_draw, &ground_sink}, *e.resources, place);

    for (auto &object : iter(place.objects))
    {
        const char *type_name = object_type_name(object.type);
        bool named_prop = object.type == Object::Type::prop && object.id[0];

        int node = add_node(e, named_prop ? object.id : type_name,
                            place_object_matrix(object));
        e.scenes[scene].nodes.push_back(node);

        NodeSink sink = {&e, node};
        emit_place_object({add_draw, &sink}, *e.resources, object,
                          Matrix4::identity());

        std::string extras = "{\"type\":";
        append_json_string(extras, type_name);
        appendf(extras, ",\"x\":%.9g,\"y\":%.9g,\"floors\":%d", object.x,
                object.y, object.floors);
        if (object.type == Object::Type::prop)
        {
            extras += ",\"id\":";
            append_json_string(extras, object.id);
        }
        if (object.place)
        {
            char interior_name[128];
            snprintf(interior_name, sizeof interior_name,
                     "%s interior (%g, %g)", type_name, object.x, object.y);
            int interior_scene = export_place(e, *object.place, interior_name);
            appendf(extras, ",\"interior_scene\":%d", interior_scene);

            if (e.options.single_scene_interiors)
            {
                e.interior_count++;
                int interior_root = export_interior_node(
                    e, *object.place, interior_name, e.interior_count);
                e.scenes[0].nodes.push_back(interior_root);
                appendf(extras, ",\"interior_node\":%d", interior_root);
            }
        }
        extras += '}';
        e.nodes[node].extras = extras;
    }

    return scene;
}

static const char *script_node_type_name(ScriptNode::Type type)
{
    switch (type)
    {
    case ScriptNode::Type::event:
        return "event";
    case ScriptNode::Type::say:
        return "say";
    case ScriptNode::Type::yesno:
        return "yesno";
    case ScriptNode::Type::teleport:
        return "teleport";
    case ScriptNode::Type::give_item:
        return "give_item";
    default:
        return "unknown";
    }
}

static const char *script_event_type_name(ScriptNode::EventType event)
{
    switch (event)
    {
    case ScriptNode::EventType::start:
        return "start";
    case ScriptNode::EventType::enter:
        return "enter";
    case ScriptNode::EventType::yes:
        return "yes";
    case ScriptNode::EventType::no:
        return "no";
    default:
        return "none";
    }
}

static const char *script_node_text(const ScriptNode &node)
{
    if (node.type == ScriptNode::Type::say ||
        node.type == ScriptNode::Type::give_item)
    {
        return node.say;
    }
    else if (node.type == ScriptNode::Type::yesno)
    {
        return node.yesno.question;
    }
    return "";
}

static std::string write_json(const GltfExport &e)
{
    std::string json =
        "{\"asset\":{\"version\":\"2.0\",\"generator\":\"Educore\"},"
        "\"extensionsUsed\":[\"KHR_lights_punctual\"],"
        "\"extensions\":{\"KHR_lights_punctual\":{\"lights\":[";
    appendf(json,
            "{\"name\":\"sun\",\"type\":\"directional\",\"color\":[%.9g,%.9g,%.9g],"
            "\"intensity\":1.0}",
            e.sun_color.x, e.sun_color.y, e.sun_color.z);
    json += "]}},\"scene\":0,\"scenes\":[";
    for (size_t i = 0; i < e.scenes.size(); i++)
    {
        json += i ? ",{\"name\":" : "{\"name\":";
        append_json_string(json, e.scenes[i].name.c_str());
        json += ",\"nodes\":";
        append_int_array(json, e.scenes[i].nodes);
        if (!e.scenes[i].extras.empty())
        {
            json += ",\"extras\":" + e.scenes[i].extras;
        }
        json += '}';
    }

    json += "],\"nodes\":[";
    for (size_t i = 0; i < e.nodes.size(); i++)
    {
        const GltfNode &node = e.nodes[i];
        json += i ? ",{\"name\":" : "{\"name\":";
        append_json_string(json, node.name.c_str());
        if (!is_identity(node.matrix))
        {
            json += ",\"matrix\":[";
            for (int j = 0; j < 16; j++)
            {
                appendf(json, j ? ",%.9g" : "%.9g", node.matrix.values[j]);
            }
            json += ']';
        }
        if (node.mesh >= 0)
        {
            appendf(json, ",\"mesh\":%d", node.mesh);
        }
        if (!node.children.empty())
        {
            json += ",\"children\":";
            append_int_array(json, node.children);
        }
        if (!node.extras.empty())
        {
            json += ",\"extras\":" + node.extras;
        }
        json += '}';
    }
    json += ']';

    if (!e.meshes.empty())
    {
        json += ",\"meshes\":[";
        for (size_t i = 0; i < e.meshes.size(); i++)
        {
            const GltfMesh &mesh = e.meshes[i];
            const GltfGeometry &geometry = e.geometries[mesh.geometry];
            json += i ? ",{\"name\":" : "{\"name\":";
            append_json_string(json, mesh.name.c_str());
            appendf(json, ",\"primitives\":[{\"attributes\":{\"POSITION\":%d",
                    geometry.position);
            if (geometry.normal >= 0)
            {
                appendf(json, ",\"NORMAL\":%d", geometry.normal);
            }
            if (geometry.uv >= 0)
            {
                appendf(json, ",\"TEXCOORD_0\":%d", geometry.uv);
            }
            appendf(json, "},\"indices\":%d,\"material\":%d}]}",
                    geometry.indices, mesh.material);
        }
        json += ']';
    }

    if (!e.materials.empty())
    {
        json += ",\"materials\":[";
        for (size_t i = 0; i < e.materials.size(); i++)
        {
            const GltfMaterial &material = e.materials[i];
            // The editor's colors are in sRGB, glTF's color factors are linear.
            appendf(json,
                    "%s{\"pbrMetallicRoughness\":{\"baseColorFactor\":"
                    "[%.9g,%.9g,%.9g,%.9g],",
                    i ? "," : "", srgb_to_linear(material.color.x),
                    srgb_to_linear(material.color.y),
                    srgb_to_linear(material.color.z), material.color.w);
            if (material.image >= 0)
            {
                appendf(json, "\"baseColorTexture\":{\"index\":%d},",
                        material.image);
            }
            json += "\"metallicFactor\":0,\"roughnessFactor\":1}";
            if (material.color.w < 1)
            {
                json += ",\"alphaMode\":\"BLEND\"";
            }
            json += '}';
        }
        json += ']';
    }

    if (!e.images.empty())
    {
        // Matches the editor's sampler.
        appendf(json, ",\"samplers\":[{\"magFilter\":%d,\"minFilter\":%d}]",
                GLTF_LINEAR, GLTF_LINEAR);
        json += ",\"textures\":[";
        for (size_t i = 0; i < e.images.size(); i++)
        {
            appendf(json, "%s{\"sampler\":0,\"source\":%zu}", i ? "," : "", i);
        }
        json += "],\"images\":[";
        for (size_t i = 0; i < e.images.size(); i++)
        {
            const char *path = e.images[i].c_str();
            const char *slash = strrchr(path, '/');
            json += i ? ",{\"name\":" : "{\"name\":";
            append_json_string(json, slash ? slash + 1 : path);
            appendf(json, ",\"bufferView\":%d,\"mimeType\":\"image/png\"}",
                    e.image_views[i]);
        }
        json += ']';
    }

    if (!e.bin.empty())
    {
        json += ",\"accessors\":[";
        for (size_t i = 0; i < e.accessors.size(); i++)
        {
            json += (i ? "," : "") + e.accessors[i];
        }
        json += "],\"bufferViews\":[";
        for (size_t i = 0; i < e.buffer_views.size(); i++)
        {
            json += (i ? "," : "") + e.buffer_views[i];
        }
        appendf(json, "],\"buffers\":[{\"byteLength\":%zu}]", e.bin.size());
    }

    json += '}';
    return json;
}

static void write_u32(FILE *f, uint32_t value)
{
    uint8_t bytes[4] = {(uint8_t)value, (uint8_t)(value >> 8),
                        (uint8_t)(value >> 16), (uint8_t)(value >> 24)};
    fwrite(bytes, 1, 4, f);
}

bool export_world_gltf(const char *path, World &world,
                       const GenResources &resources,
                       GltfExportOptions options)
{
    GltfExport e = {};
    e.resources = &resources;
    e.options = options;
    export_place(e, *world.first, "World");

    float t = world.time_of_day / 24.0f;
    float angle = t * 3.14159265f * 2.0f;

    float sun_x = cosf(angle);
    float sun_y = -sinf(angle);
    float sun_z = 0.5f;

    // Mirrored Z for glTF's right-handed coordinate system
    float dir_x = -sun_x;
    float dir_y = -sun_y;
    float dir_z = -sun_z;

    float day_intensity = sinf(angle - 3.14159265f / 2.0f) * 0.5f + 0.5f;

    Vector3 day_light = {0.9f, 0.9f, 0.8f};
    Vector3 night_light = {0.1f, 0.2f, 0.4f};
    Vector3 sunset_light = {1.0f, 0.5f, 0.2f};

    Vector3 light_color;
    if (day_intensity > 0.5f)
    {
        float f = (day_intensity - 0.5f) * 2.0f;
        light_color = {
            sunset_light.x + (day_light.x - sunset_light.x) * f,
            sunset_light.y + (day_light.y - sunset_light.y) * f,
            sunset_light.z + (day_light.z - sunset_light.z) * f};
    }
    else
    {
        float f = day_intensity * 2.0f;
        light_color = {
            night_light.x + (sunset_light.x - night_light.x) * f,
            night_light.y + (sunset_light.y - night_light.y) * f,
            night_light.z + (sunset_light.z - night_light.z) * f};
    }

    e.sun_color = light_color;

    std::string script_json = "[";
    if (world.script)
    {
        bool first_node = true;
        for (auto &node : iter(world.script->nodes))
        {
            if (!first_node)
            {
                script_json += ',';
            }
            first_node = false;

            script_json += "{\"type\":";
            append_json_string(script_json, script_node_type_name(node.type));
            script_json += ",\"event\":";
            append_json_string(script_json, script_event_type_name(node.event));
            script_json += ",\"text\":";
            append_json_string(script_json, script_node_text(node));
            script_json += '}';
        }
    }
    script_json += ']';

    std::string scene_extras = "{\"time_of_day\":";
    appendf(scene_extras, "%.9g", world.time_of_day);
    scene_extras += ",\"sun_light\":{\"direction\":[";
    appendf(scene_extras, "%.9g,%.9g,%.9g", dir_x, dir_y, dir_z);
    scene_extras += "],\"color\":[";
    appendf(scene_extras, "%.9g,%.9g,%.9g", light_color.x, light_color.y,
            light_color.z);
    scene_extras += "]},\"script\":";
    scene_extras += script_json;
    scene_extras += '}';

    if (!e.scenes.empty())
    {
        e.scenes[0].extras = scene_extras;
    }

    std::string json = write_json(e);
    while (json.size() % 4)
    {
        json += ' ';
    }
    while (e.bin.size() % 4)
    {
        e.bin.push_back(0);
    }

    FILE *f = fopen(path, "wb");
    if (!f)
    {
        fprintf(stderr, "glTF export: failed to open '%s'\n", path);
        return false;
    }

    size_t length = 12 + 8 + json.size();
    if (!e.bin.empty())
    {
        length += 8 + e.bin.size();
    }

    write_u32(f, 0x46546C67); // "glTF"
    write_u32(f, 2);
    write_u32(f, (uint32_t)length);

    write_u32(f, (uint32_t)json.size());
    write_u32(f, 0x4E4F534A); // "JSON"
    fwrite(json.data(), 1, json.size(), f);

    if (!e.bin.empty())
    {
        write_u32(f, (uint32_t)e.bin.size());
        write_u32(f, 0x004E4942); // "BIN"
        fwrite(e.bin.data(), 1, e.bin.size(), f);
    }

    bool ok = !ferror(f);
    ok = fclose(f) == 0 && ok;
    return ok;
}

bool export_world_gltf(const char *path, World &world,
                       const GenResources &resources)
{
    return export_world_gltf(path, world, resources, GltfExportOptions{});
}
