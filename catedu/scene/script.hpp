#pragma once
#include "catedu/core/alloc/arena.hpp"
#include "catedu/core/alloc/free_list.hpp"
#include "catedu/core/storage/stack.hpp"

struct Place;

struct ScriptNode
{
    enum class EventType
    {
        start,
        enter,
        yes,
        no
    };

    enum class Type
    {
        event,
        say,
        yesno,
        teleport,
        count_
    };

    EventType event;
    Place *place;
    Type type;

    union {
        char say[256];
        struct
        {
            char question[256];
            ScriptNode *yes;
            ScriptNode *no;
        } yesno;
        struct
        {
            Place *target;
        } teleport;
    };

    ScriptNode *parent;
    ScriptNode *next;
};

struct Script
{
    FreeList<ScriptNode> nodes;
    Stack<ScriptNode *> global_events;
    ScriptNode *root;

    ScriptNode *append_node(ScriptNode::Type t, ScriptNode *parent);

    Script clone();

    ScriptNode *create_start_event();
    ScriptNode *create_place_event(Place *place);

    ScriptNode *get_start_event();
    ScriptNode *get_place_event(Place *place);

    ScriptNode *acquire_start_event();
    ScriptNode *acquire_place_event(Place *place);

    static Script create(Arena arena);
    void destroy();
};
