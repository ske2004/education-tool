#include "basic_tilemap.hpp"

static ChunkPositionToChunk *get_chunk(BasicTilemap *tilemap,
                                       Vector2i chunk_position)
{
    for (auto &chunk : iter(tilemap->chunks))
    {
        if (chunk.chunk_position == chunk_position)
        {
            return &chunk;
        }
    }

    return nullptr;
}

static ChunkPositionToChunk &get_or_create_chunk(BasicTilemap *tilemap,
                                                 Vector2i chunk_position)
{
    for (auto &chunk : iter(tilemap->chunks))
    {
        if (chunk.chunk_position == chunk_position)
        {
            return chunk;
        }
    }

    ChunkPositionToChunk *c = tilemap->chunks.alloc();
    *c = {chunk_position, {}};
    return *c;
}

static Vector2i chunk_tile_position_to_tile_position(Vector2i chunk_position,
                                                     int tile_id)
{
    Vector2i chunk_tile_position = {tile_id % BASIC_TILEMAP_CHUNK_DIM,
                                    tile_id / BASIC_TILEMAP_CHUNK_DIM};
    return chunk_position * BASIC_TILEMAP_CHUNK_DIM + chunk_tile_position;
}

BasicTilemapSerial BasicTilemapSerial::init(BasicTilemap &tilemap)
{
    BasicTilemapSerial serial = {};
    serial.tilemap = &tilemap;
    return serial;
}

void BasicTilemapSerial::deinit()
{
}

TilePositionToTile BasicTilemapSerial::next()
{
    int ci = 0;
    for (auto &chunk : iter(tilemap->chunks))
    {
        if (ci < chunk_id)
        {
            ci++;
            continue;
        }

        for (; tile_id < BASIC_TILEMAP_CHUNK_SIZE; tile_id++)
        {
            if (chunk.chunk.data[tile_id] != 0)
            {
                TilePositionToTile tile = {};
                tile.position = chunk_tile_position_to_tile_position(
                    chunk.chunk_position, tile_id);
                tile.id = chunk.chunk.data[tile_id];
                tile_id++;
                chunk_id = ci;
                return tile;
            }
        }
        tile_id = 0;
        ci++;
        chunk_id = ci;
    }

    TilePositionToTile tile = {};
    tile.id = -1;
    return tile;
}

BasicTilemap BasicTilemap::init()
{
    BasicTilemap tilemap = {};
    tilemap.chunks = FreeList<ChunkPositionToChunk>::create(
        Arena::create(&ALLOCATOR_MALLOC));
    return tilemap;
}

void BasicTilemap::deinit()
{
    this->chunks.destroy();
}

void BasicTilemap::set_tile(Vector2i tile_position, int tile_id)
{
    Vector2i chunk_position = tile_position / BASIC_TILEMAP_CHUNK_DIM;
    Vector2i chunk_tile_position = tile_position % BASIC_TILEMAP_CHUNK_DIM;

    // Negative modulo is not what we want
    if (chunk_tile_position.x < 0)
    {
        chunk_tile_position.x += BASIC_TILEMAP_CHUNK_DIM;
        chunk_position.x -= 1;
    }
    if (chunk_tile_position.y < 0)
    {
        chunk_tile_position.y += BASIC_TILEMAP_CHUNK_DIM;
        chunk_position.y -= 1;
    }

    ChunkPositionToChunk &chunk = get_or_create_chunk(this, chunk_position);
    chunk.chunk.data[chunk_tile_position.y * BASIC_TILEMAP_CHUNK_DIM +
                     chunk_tile_position.x] = tile_id;
}

int BasicTilemap::get_tile(Vector2i tile_position)
{
    Vector2i chunk_position = tile_position / BASIC_TILEMAP_CHUNK_DIM;
    Vector2i chunk_tile_position = tile_position % BASIC_TILEMAP_CHUNK_DIM;

    // Negative modulo is not what we want
    if (chunk_tile_position.x < 0)
    {
        chunk_tile_position.x += BASIC_TILEMAP_CHUNK_DIM;
        chunk_position.x -= 1;
    }
    if (chunk_tile_position.y < 0)
    {
        chunk_tile_position.y += BASIC_TILEMAP_CHUNK_DIM;
        chunk_position.y -= 1;
    }

    ChunkPositionToChunk *chunk = get_chunk(this, chunk_position);
    if (!chunk)
    {
        return 0;
    }
    return chunk->chunk.data[chunk_tile_position.y * BASIC_TILEMAP_CHUNK_DIM +
                             chunk_tile_position.x];
}
