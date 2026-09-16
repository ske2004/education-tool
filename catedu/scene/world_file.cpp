#include "world_file.hpp"
#include <stdio.h>
#include <vector>

// TODO: Get rid of this and use handles instead
struct PtrToHandle
{
    void *ptr;
    uint32_t handle;
};

struct Mapper
{
    std::vector<PtrToHandle> mappings;
};

struct SavingObject
{
    Object::Type type;
    int floors;
    float x, y;
    uint32_t place;
};

void pushmapper(Mapper *mapper, void *ptr, uint32_t handle)
{
    mapper->mappings.push_back({ptr, handle + 1});
}

uint32_t gethandle(Mapper *mapper, void *ptr)
{
    for (size_t i = 0; i < mapper->mappings.size(); i++)
    {
        if (mapper->mappings[i].ptr == ptr)
        {
            return mapper->mappings[i].handle;
        }
    }

    return 0;
}

void *getptr(Mapper *mapper, uint32_t handle)
{
    for (size_t i = 0; i < mapper->mappings.size(); i++)
    {
        if (mapper->mappings[i].handle == handle)
        {
            return mapper->mappings[i].ptr;
        }
    }

    return nullptr;
}

bool WorldFile::save(const char *path, Dispatcher &dispatcher)
{
    Mapper mapper = {};

    FILE *file = fopen(path, "wb");
    if (!file)
    {
        fprintf(stderr, "WorldFile::save: failed to open '%s' for writing\n", path);
        return false;
    }

    for (auto &place : iter(dispatcher.world.places))
    {
        pushmapper(&mapper, &place, mapper.mappings.size());
    }

    uint32_t place_count = mapper.mappings.size();
    fwrite(&place_count, sizeof(place_count), 1, file);

    for (auto &place : iter(dispatcher.world.places))
    {
        uint32_t count = 0;
        for (auto &_ : iter(place.objects))
        {
            count++;
        }

        fwrite(&count, sizeof(count), 1, file);
        fwrite(&place.interior, sizeof(place.interior), 1, file);

        for (auto &obj : iter(place.objects))
        {
            SavingObject so = {};
            so.type = obj.type;
            so.floors = obj.floors;
            so.x = obj.x;
            so.y = obj.y;
            so.place = gethandle(&mapper, obj.place);

            fwrite(&so, sizeof(so), 1, file);
        }
    }

    // uint32_t node_count = 0;
    // for (auto &_ : iter(dispatcher.world.script.nodes))
    // {
    //     node_count++;
    // }

    // fwrite(&node_count, sizeof(node_count), 1, file);

    // for (auto &node : iter(dispatcher.world.script.nodes))
    // {
    //     fwrite(&node, sizeof(node), 1, file);
    // }

    fclose(file);

    dispatcher.dirty = false;
    return true;
}

Dispatcher WorldFile::load(const char *path)
{
    Mapper mapper = {};

    FILE *file = fopen(path, "rb");

    Dispatcher dispatcher = Dispatcher::create();

#if 1
    return dispatcher;
#endif

    if (!file)
    {
        return dispatcher;
    }

    uint32_t count = 0;
    fread(&count, sizeof(count), 1, file);

    for (size_t i = 0; i < count; i++)
    {
        Place *place;
        if (i == 0)
        {
            place = dispatcher.world.first;
        }
        else
        {
            place = dispatcher.world.places.alloc();
            *place = Place::create();
        }

        pushmapper(&mapper, place, i);
    }

    for (size_t i = 1; i <= count; i++)
    {
        Place *place = (Place *)getptr(&mapper, i);

        uint32_t count = 0;
        fread(&count, sizeof(count), 1, file);
        fread(&place->interior, sizeof(place->interior), 1, file);

        for (size_t i = 0; i < count; i++)
        {
            SavingObject so = {};
            fread(&so, sizeof(so), 1, file);

            Object obj = {};
            obj.type = so.type;
            obj.floors = so.floors;
            obj.x = so.x;
            obj.y = so.y;
            obj.place = (Place *)getptr(&mapper, so.place);
            place->place_object(obj);
        }
    }

    // uint32_t node_count = 0;
    // fread(&node_count, sizeof(node_count), 1, file);

    // for (size_t i = 0; i < node_count; i++)
    // {
    //     ScriptNode node = {};
    //     fread(&node, sizeof(node), 1, file);
    //     dispatcher.world.script.nodes.alloc(node);
    // }

    fclose(file);

    return dispatcher;
}
