#include "model.hpp"
#include "catedu/sys/fs/read_file_temp.hpp"
#include "cgltf/cgltf.h"

#include <catedu/core/alloc/allocator.hpp>
#include <stdlib.h>
#include <string.h>

float *merge_buffers(float *pos, float *norm, float *uv, size_t vertex_count)
{
    float *result =
        (float *)ALLOCATOR_MALLOC.alloc(vertex_count * 8 * sizeof(float));

    for (size_t i = 0; i < vertex_count; i++)
    {
        result[i * 8 + 0] = pos[i * 3 + 0];
        result[i * 8 + 1] = pos[i * 3 + 1];
        result[i * 8 + 2] = pos[i * 3 + 2];
        result[i * 8 + 3] = norm[i * 3 + 0];
        result[i * 8 + 4] = norm[i * 3 + 1];
        result[i * 8 + 5] = norm[i * 3 + 2];
        result[i * 8 + 6] = uv[i * 2 + 0];
        result[i * 8 + 7] = uv[i * 2 + 1];
    }
    return result;
}

bool RawModel::load_gltf(const char *path, RawModel &dest)
{
    cgltf_data *data = NULL;
    cgltf_options options = {};

    dest = {};

    READ_FILE_TEMP(buf, path, {
        cgltf_result result = cgltf_parse(&options, buf.data, buf.size, &data);

        if (result != cgltf_result_success)
        {
            fprintf(stderr, "Failed to parse gltf: %d\n", result);
            return false;
        }
    });

    cgltf_result result = cgltf_load_buffers(&options, data, path);
    if (result != cgltf_result_success)
    {
        fprintf(stderr, "Failed to load gltf buffers: %d\n", result);
        return false;
    }

    assert(data->images_count <= 1 && "Only one image is supported for now");
    if (data->images_count > 0)
    {
        dest.texture_path = (char *)ALLOCATOR_MALLOC.alloc(1024);
        memset(dest.texture_path, 0, 1024);

        strncpy(dest.texture_path, path, 1024);

        const char *s0 = strrchr(path, '/');
        const char *s1 = strrchr(path, '\\');
        const char *slash = s0 ? (s1 && s1 > s0 ? s1 : s0) : s1;

        snprintf(dest.texture_path, 1024, "%.*s/%s", (int)(slash - path), path,
                 data->images[0].uri);
    }
    dest.data = data;
    return true;
}

void RawModel::deinit()
{
    ALLOCATOR_MALLOC.free(texture_path);
    cgltf_free(data);
}

void print_info(RawModel &model)
{
    cgltf_data *data = model.data;

    fprintf(stderr, "Buffer view count: %zu\n", data->buffer_views_count);
    for (size_t i = 0; i < data->buffer_views_count; i++)
    {
        cgltf_buffer_view *view = &data->buffer_views[i];
        fprintf(stderr, "Buffer view %zu: buffer %p, offset %zu, size %zu\n", i,
                view->buffer, view->offset, view->size);
    }

    fprintf(stderr, "Meshes count: %zu\n", data->meshes_count);
    for (size_t i = 0; i < data->meshes_count; i++)
    {
        cgltf_mesh *mesh = &data->meshes[i];
        fprintf(stderr, "Mesh %zu: name %s, primitives count %zu\n", i,
                mesh->name, mesh->primitives_count);

        for (size_t j = 0; j < mesh->primitives_count; j++)
        {
            cgltf_primitive *prim = &mesh->primitives[j];
            fprintf(stderr, "Primitive %zu: type %i, indices %p, material %p\n",
                    j, prim->type, prim->indices, prim->material);

            for (size_t k = 0; k < prim->attributes_count; k++)
            {
                cgltf_attribute *attr = &prim->attributes[k];

                fprintf(stderr,
                        "Attribute %zu: name %s, type %i, data %p, stride: "
                        "%zu, offset: %zu\n",
                        k, attr->name, attr->type, attr->data,
                        attr->data->buffer_view->stride,
                        attr->data->buffer_view->offset);
            }
        }
    }
}

bool Model::load_from_raw(RawModel &raw, Model &dest, int submodel)
{
    cgltf_data *data = raw.data;

    assert(data->buffers_count == 1 &&
           "Multiple buffers are not supported yet");
    assert(data->buffers[0].data && "Buffer data is required");
    assert(data->meshes_count == 1 && "Multiple meshes are not supported yet");

    cgltf_mesh *mesh = &data->meshes[0];

    assert(mesh->primitives_count == 1 &&
           "Multiple primitives are not supported yet");

    cgltf_primitive *prim = &mesh->primitives[0];

    assert(prim->type == cgltf_primitive_type_triangles &&
           "Only triangles are supported");

    assert(prim->indices->component_type == cgltf_component_type_r_16u &&
           "Only 16-bit indices are supported");

    uint16_t *indices =
        (uint16_t *)((uint8_t *)prim->indices->buffer_view->buffer->data +
                     prim->indices->buffer_view->offset);

    assert(prim->attributes_count == 3 &&
           "3 attributes required (pos, norm, uv)");

    float *pos = NULL;
    float *norm = NULL;
    float *uv = NULL;
    size_t vertex_count = 0;
    size_t index_count = prim->indices->count;

    for (size_t i = 0; i < prim->attributes_count; i++)
    {
        cgltf_attribute *attr = &prim->attributes[i];

        assert(attr->data->component_type == cgltf_component_type_r_32f &&
               "Only 32-bit floats are supported");

        switch (attr->type)
        {
        case cgltf_attribute_type_position:
            pos = (float *)((uint8_t *)attr->data->buffer_view->buffer->data +
                            attr->data->buffer_view->offset);
            break;
        case cgltf_attribute_type_normal:
            norm = (float *)((uint8_t *)attr->data->buffer_view->buffer->data +
                             attr->data->buffer_view->offset);
            break;
        case cgltf_attribute_type_texcoord:
            uv = (float *)((uint8_t *)attr->data->buffer_view->buffer->data +
                           attr->data->buffer_view->offset);
            break;
        default:
            assert(false && "Unknown attribute type");
        }

        vertex_count = attr->data->count;
    }

    assert(indices && "Indices are required");
    assert(pos && "Position is required");
    assert(norm && "Normal is required");
    assert(uv && "UV is required");

    fprintf(stderr, "Vertex count: %zu\n", vertex_count);
    fprintf(stderr, "Index count: %zu\n", index_count);
    fprintf(stderr, "POS: %p\n", pos);
    fprintf(stderr, "NORM: %p\n", norm);
    fprintf(stderr, "UV: %p\n", uv);

    float *buf = merge_buffers(pos, norm, uv, vertex_count);

    sg_buffer_desc vertex_buffer_desc = {};
    vertex_buffer_desc.data = {buf, vertex_count * 8 * sizeof(float)};
    vertex_buffer_desc.label = "model-vertex-buffer";
    vertex_buffer_desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
    dest.vertex_buffer = sg_make_buffer(vertex_buffer_desc);

    ALLOCATOR_MALLOC.free(buf);

    sg_buffer_desc index_buffer_desc = {};
    index_buffer_desc.data = {indices, index_count * sizeof(uint16_t)};
    index_buffer_desc.label = "model-index-buffer";
    index_buffer_desc.type = SG_BUFFERTYPE_INDEXBUFFER;
    dest.index_buffer = sg_make_buffer(index_buffer_desc);

    dest.index_count = index_count;

    if (raw.texture_path)
    {
        if (submodel != 0)
        {
            // TODO: Dynamic strings here.
            char path[1024] = {};
            int pos = strrchr(raw.texture_path, '.') - raw.texture_path;
            assert(pos > 0 && "Invalid texture path");
            snprintf(path, 1024, "%.*s_%i%s", pos, raw.texture_path, submodel,
                     raw.texture_path + pos);
            printf("loading texture: %s\n", path);
            dest.texture = Texture::init(path);
        }
        else
        {
            dest.texture = Texture::init(raw.texture_path);
        }
    }

    return true;
}

void Model::deinit()
{
    sg_destroy_buffer(vertex_buffer);
    sg_destroy_buffer(index_buffer);
    texture.deinit();
}
