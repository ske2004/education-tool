#include "world_file.hpp"
#include "catedu/sys/json_fields.hpp"
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

// TODO: Get rid of this and use handles instead
static const char WORLD_FILE_MAGIC[4] = {'E', 'D', 'U', 'W'};
// Version 1 is the legacy headerless layout.
static const uint32_t WORLD_FILE_VERSION = 2;

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
    char id[32];
};

struct SavingScriptNode
{
    ScriptNode::EventType event;
    uint32_t place_handle;
    ScriptNode::Type type;
    char text[256];
    uint32_t yes_handle;
    uint32_t no_handle;
    uint32_t parent_handle;
    uint32_t next_handle;
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

static void export_place(FILE *f, Place *place) {
    fprintf(f, "[\n");
    bool first = true;
    for (auto &obj : iter(place->objects)) {
        if (!first) fprintf(f, ",\n");
        first = false;
        
        const char *type_name = object_type_name(obj.type);
        
        fprintf(f, "    {\"type\": \"%s\", \"x\": %f, \"y\": %f, \"floors\": %d", 
                type_name, obj.x, obj.y, obj.floors);
                
        if (obj.type == Object::Type::prop) {
            std::string escaped_id;
            append_json_string(escaped_id, obj.id);
            fprintf(f, ", \"id\": %s", escaped_id.c_str());
        }
                
        if (obj.place) {
            fprintf(f, ", \"interior\": ");
            export_place(f, obj.place);
        }
        fprintf(f, "}");
    }
    fprintf(f, "\n  ]");
}

void WorldFile::export_json(const char *path, Dispatcher &dispatcher)
{
    FILE *f = fopen(path, "w");
    if (!f) return;
    
    fprintf(f, "{\n  \"world\": ");
    export_place(f, dispatcher.world.first);
    fprintf(f, "\n}\n");
    fclose(f);
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

    fwrite(WORLD_FILE_MAGIC, sizeof(WORLD_FILE_MAGIC), 1, file);
    uint32_t version = WORLD_FILE_VERSION;
    fwrite(&version, sizeof(version), 1, file);

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
            memcpy(so.id, obj.id, sizeof(so.id));

            fwrite(&so, sizeof(so), 1, file);
        }
    }

    Mapper node_mapper = {};
    uint32_t node_count = 0;
    for (auto &node : iter(dispatcher.world.script->nodes))
    {
        pushmapper(&node_mapper, &node, node_mapper.mappings.size());
        node_count++;
    }

    fwrite(&node_count, sizeof(node_count), 1, file);

    for (auto &node : iter(dispatcher.world.script->nodes))
    {
        SavingScriptNode sn = {};
        sn.event = node.event;
        sn.place_handle = gethandle(&mapper, node.place);
        sn.type = node.type;

        if (node.type == ScriptNode::Type::yesno)
        {
            strncpy(sn.text, node.yesno.question, sizeof(sn.text) - 1);
            sn.yes_handle = gethandle(&node_mapper, node.yesno.yes);
            sn.no_handle = gethandle(&node_mapper, node.yesno.no);
        }
        else if (node.type == ScriptNode::Type::teleport)
        {
            sn.yes_handle = gethandle(&mapper, node.teleport.target);
        }
        else
        {
            strncpy(sn.text, node.say, sizeof(sn.text) - 1);
        }

        sn.parent_handle = gethandle(&node_mapper, node.parent);
        sn.next_handle = gethandle(&node_mapper, node.next);

        fwrite(&sn, sizeof(sn), 1, file);
    }

    uint32_t global_events_count = dispatcher.world.script->global_events.count;
    fwrite(&global_events_count, sizeof(global_events_count), 1, file);

    for (auto &ev : iter(dispatcher.world.script->global_events))
    {
        uint32_t handle = gethandle(&node_mapper, ev);
        fwrite(&handle, sizeof(handle), 1, file);
    }

    uint32_t root_handle = gethandle(&node_mapper, dispatcher.world.script->root);
    fwrite(&root_handle, sizeof(root_handle), 1, file);

    fclose(file);

    dispatcher.dirty = false;
    return true;
}

Dispatcher WorldFile::load(const char *path)
{
    Mapper mapper = {};

    Dispatcher dispatcher = Dispatcher::create();

    FILE *file = fopen(path, "rb");
    if (!file)
    {
        return dispatcher;
    }

    char magic[4] = {};
    if (fread(magic, sizeof(magic), 1, file) == 1 &&
        memcmp(magic, WORLD_FILE_MAGIC, sizeof(magic)) == 0)
    {
        uint32_t version = 0;
        if (fread(&version, sizeof(version), 1, file) != 1 || version < 2 ||
            version > WORLD_FILE_VERSION)
        {
            fprintf(stderr,
                    "WorldFile::load: '%s' has unsupported format version %u "
                    "(this build supports up to %u)\n",
                    path, version, WORLD_FILE_VERSION);
            fclose(file);
            return dispatcher;
        }
    }
    else
    {
        // Legacy headerless v1 file.
        rewind(file);
    }

    uint32_t count = 0;
    if (fread(&count, sizeof(count), 1, file) != 1 || count > 10000)
    {
        fclose(file);
        return dispatcher;
    }

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

        uint32_t obj_count = 0;
        if (fread(&obj_count, sizeof(obj_count), 1, file) != 1 ||
            fread(&place->interior, sizeof(place->interior), 1, file) != 1 ||
            obj_count > 100000)
        {
            fclose(file);
            return dispatcher;
        }

        for (size_t j = 0; j < obj_count; j++)
        {
            SavingObject so = {};
            if (fread(&so, sizeof(so), 1, file) != 1)
            {
                fclose(file);
                return dispatcher;
            }

            if ((int)so.type < 0 || (int)so.type > (int)Object::Type::castle)
            {
                continue;
            }

            Object obj = {};
            obj.type = so.type;
            obj.floors = so.floors;
            obj.x = so.x;
            obj.y = so.y;
            obj.place = (Place *)getptr(&mapper, so.place);
            memcpy(obj.id, so.id, sizeof(obj.id));
            obj.id[sizeof(obj.id) - 1] = '\0';
            place->place_object(obj);
        }
    }

    uint32_t node_count = 0;
    if (fread(&node_count, sizeof(node_count), 1, file) != 1 || node_count > 100000)
    {
        // Old file with no script section — keep default empty script.
        fclose(file);
        return dispatcher;
    }

    std::vector<SavingScriptNode> saved_nodes(node_count);
    if (node_count > 0)
    {
        if (fread(saved_nodes.data(), sizeof(SavingScriptNode), node_count, file) != node_count)
        {
            fclose(file);
            return dispatcher;
        }
    }

    // Replace default empty script with a fresh one.
    dispatcher.world.script->destroy();
    *dispatcher.world.script = Script::create(Arena::create(&ALLOCATOR_MALLOC));

    Mapper node_mapper = {};

    // First pass: allocate nodes and populate non-pointer fields.
    for (uint32_t i = 0; i < node_count; i++)
    {
        ScriptNode *node = dispatcher.world.script->nodes.alloc();
        *node = {};
        node->event = saved_nodes[i].event;
        node->type = saved_nodes[i].type;
        if ((int)node->type < 0 || (int)node->type >= (int)ScriptNode::Type::count_)
        {
            node->type = ScriptNode::Type::say;
        }

        if (node->type == ScriptNode::Type::yesno)
        {
            strncpy(node->yesno.question, saved_nodes[i].text, sizeof(node->yesno.question) - 1);
        }
        else if (node->type == ScriptNode::Type::teleport)
        {
            // No text data for teleport nodes.
        }
        else
        {
            strncpy(node->say, saved_nodes[i].text, sizeof(node->say) - 1);
        }

        pushmapper(&node_mapper, node, i);
    }

    // Second pass: remap handles back to pointers.
    uint32_t idx = 0;
    for (auto &node : iter(dispatcher.world.script->nodes))
    {
        node.parent = (ScriptNode *)getptr(&node_mapper, saved_nodes[idx].parent_handle);
        node.next = (ScriptNode *)getptr(&node_mapper, saved_nodes[idx].next_handle);
        node.place = (Place *)getptr(&mapper, saved_nodes[idx].place_handle);

        if (node.type == ScriptNode::Type::yesno)
        {
            node.yesno.yes = (ScriptNode *)getptr(&node_mapper, saved_nodes[idx].yes_handle);
            node.yesno.no = (ScriptNode *)getptr(&node_mapper, saved_nodes[idx].no_handle);
        }
        else if (node.type == ScriptNode::Type::teleport)
        {
            node.teleport.target = (Place *)getptr(&mapper, saved_nodes[idx].yes_handle);
        }

        idx++;
    }

    uint32_t global_events_count = 0;
    if (fread(&global_events_count, sizeof(global_events_count), 1, file) == 1 &&
        global_events_count <= 100000)
    {
        for (uint32_t i = 0; i < global_events_count; i++)
        {
            uint32_t handle = 0;
            if (fread(&handle, sizeof(handle), 1, file) != 1)
            {
                break;
            }
            ScriptNode *node = (ScriptNode *)getptr(&node_mapper, handle);
            if (node)
            {
                dispatcher.world.script->global_events.push(node);
            }
        }
    }

    uint32_t root_handle = 0;
    if (fread(&root_handle, sizeof(root_handle), 1, file) == 1)
    {
        dispatcher.world.script->root = (ScriptNode *)getptr(&node_mapper, root_handle);
    }

    fclose(file);

    return dispatcher;
}
