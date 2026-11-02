#include "world_file.hpp"
#include "catedu/sys/json_fields.hpp"
#include <stdio.h>
#include <string.h>
#include <string>
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

// File header. Files written before versioning start directly with the
// place count, which is always far smaller than this magic value.
static const uint32_t WORLD_FILE_MAGIC = 0x57554445; // "EDUW"
static const uint32_t WORLD_FILE_VERSION = 2;

// Version 0 (unversioned, before props) object record.
struct SavingObjectV0
{
    Object::Type type;
    int floors;
    float x, y;
    uint32_t place;
};

// Version 1 (unversioned, 8ba44ed onwards): adds prop id.
struct SavingObjectV1
{
    Object::Type type;
    int floors;
    float x, y;
    uint32_t place;
    char id[32];
};

// Version 2 (versioned header): adds z.
struct SavingObject
{
    Object::Type type;
    int floors;
    float x, y, z;
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
                
        if (obj.type == Object::Type::prop || obj.id[0] != '\0') {
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
    if (!f) {
        fprintf(stderr, "WorldFile::export_json: failed to open '%s' for writing\n", path);
        return;
    }
    
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

    fwrite(&WORLD_FILE_MAGIC, sizeof(WORLD_FILE_MAGIC), 1, file);
    fwrite(&WORLD_FILE_VERSION, sizeof(WORLD_FILE_VERSION), 1, file);

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
            so.z = obj.z;
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

        if (node.type == ScriptNode::Type::yesno ||
            node.type == ScriptNode::Type::check_item)
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

static bool read_u32_at(FILE *file, long offset, uint32_t *out)
{
    return fseek(file, offset, SEEK_SET) == 0 && fread(out, sizeof(*out), 1, file) == 1;
}

// Checks whether an unversioned file parses exactly to its end when objects
// are `record_size` bytes. `start` points just past the place count.
static bool legacy_layout_fits(FILE *file, long start, long file_size,
                               uint32_t place_count, size_t record_size)
{
    long pos = start;
    for (uint32_t i = 0; i < place_count; i++)
    {
        uint32_t obj_count = 0;
        if (!read_u32_at(file, pos, &obj_count) || obj_count > 100000)
            return false;
        pos += sizeof(uint32_t) + sizeof(bool) + (long)(obj_count * record_size);
        if (pos > file_size)
            return false;
    }

    // Script section is optional in old files, and so are its tail parts.
    if (pos == file_size)
        return true;

    uint32_t node_count = 0;
    if (!read_u32_at(file, pos, &node_count) || node_count > 100000)
        return false;
    pos += sizeof(uint32_t) + (long)(node_count * sizeof(SavingScriptNode));
    if (pos == file_size)
        return true;

    uint32_t global_events_count = 0;
    if (!read_u32_at(file, pos, &global_events_count) || global_events_count > 100000)
        return false;
    pos += sizeof(uint32_t) + (long)(global_events_count * sizeof(uint32_t));
    if (pos == file_size)
        return true;

    return pos + (long)sizeof(uint32_t) == file_size; // root handle
}

// Unversioned files come in two object layouts (with or without prop id).
// Picks the one under which the file parses cleanly; leaves the file
// position unchanged.
static uint32_t detect_legacy_version(FILE *file, uint32_t place_count)
{
    long start = ftell(file);
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);

    uint32_t version = 1;
    if (legacy_layout_fits(file, start, file_size, place_count, sizeof(SavingObjectV1)))
        version = 1;
    else if (legacy_layout_fits(file, start, file_size, place_count, sizeof(SavingObjectV0)))
        version = 0;

    fseek(file, start, SEEK_SET);
    return version;
}

static bool read_saving_object(FILE *file, uint32_t version, SavingObject *so)
{
    *so = {};
    if (version >= 2)
    {
        return fread(so, sizeof(*so), 1, file) == 1;
    }

    if (version == 1)
    {
        SavingObjectV1 v1 = {};
        if (fread(&v1, sizeof(v1), 1, file) != 1)
            return false;
        so->type = v1.type;
        so->floors = v1.floors;
        so->x = v1.x;
        so->y = v1.y;
        so->place = v1.place;
        memcpy(so->id, v1.id, sizeof(so->id));
        return true;
    }

    SavingObjectV0 v0 = {};
    if (fread(&v0, sizeof(v0), 1, file) != 1)
        return false;
    so->type = v0.type;
    so->floors = v0.floors;
    so->x = v0.x;
    so->y = v0.y;
    so->place = v0.place;
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

    uint32_t version = 0;
    uint32_t count = 0;
    if (fread(&count, sizeof(count), 1, file) != 1)
    {
        fclose(file);
        return dispatcher;
    }

    if (count == WORLD_FILE_MAGIC)
    {
        if (fread(&version, sizeof(version), 1, file) != 1 || version > WORLD_FILE_VERSION)
        {
            fprintf(stderr, "WorldFile::load: '%s' has an unsupported version\n", path);
            fclose(file);
            return dispatcher;
        }
        if (fread(&count, sizeof(count), 1, file) != 1)
        {
            fclose(file);
            return dispatcher;
        }
    }
    else
    {
        version = detect_legacy_version(file, count);
    }

    if (count > 10000)
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
            if (!read_saving_object(file, version, &so))
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
            obj.z = so.z;
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

        if (node->type == ScriptNode::Type::yesno ||
            node->type == ScriptNode::Type::check_item)
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

        if (node.type == ScriptNode::Type::yesno ||
            node.type == ScriptNode::Type::check_item)
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
