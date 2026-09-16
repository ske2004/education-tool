#include "mesh_generator.hpp"
#include "catedu/core/alloc/allocator.hpp"
#include <assert.h>
#include <cstring>

RenderMeshGenerator RenderMeshGenerator::init(size_t vertex_stride)
{
    RenderMeshGenerator generator = {};
    generator.vertex_stride = vertex_stride;
    return generator;
}

uint16_t RenderMeshGenerator::push_vert(float *vertex, size_t count)
{
    assert(count == vertex_stride);
    for (size_t i = 0; i < count; i++)
    {
        vertices.push_back(vertex[i]);
    }
    assert(vertices.size() >= vertex_stride && "No vertices to index");
    return vertices.size() / vertex_stride - 1;
}

void RenderMeshGenerator::push_tri(uint16_t a, uint16_t b, uint16_t c)
{
    this->indices.push_back(a);
    this->indices.push_back(b);
    this->indices.push_back(c);
}

RenderGeo RenderMeshGenerator::convert(RenderWriteDesc desc)
{
    RenderGeo geo = {};

    geo.ibuf_size = this->indices.size();
    geo.ibuf = (uint16_t *)ALLOCATOR_MALLOC.alloc(this->indices.size() *
                                                  sizeof(uint16_t));
    memcpy(geo.ibuf, this->indices.data(),
           this->indices.size() * sizeof(uint16_t));

    geo.vbuf_size = this->vertices.size() / this->vertex_stride * desc.stride;
    geo.vbuf = (float *)ALLOCATOR_MALLOC.alloc(geo.vbuf_size * sizeof(float));
    memset(geo.vbuf, 0, geo.vbuf_size * sizeof(float));
    geo.stride = desc.stride;

    for (size_t i = 0, k = 0; i < this->vertices.size();
         i += this->vertex_stride, k += desc.stride)
    {
        memcpy(geo.vbuf + k + desc.offset, this->vertices.data() + i,
               sizeof(float) * vertex_stride);
    }

    assert(geo.validate());

    return geo;
}
