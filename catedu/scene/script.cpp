#include "script.hpp"
#include "catedu/core/alloc/allocator.hpp"
#include "catedu/core/alloc/arena.hpp"
#include "catedu/core/memory/addressfixer.hpp"

ScriptNode *Script::append_node(ScriptNode::Type t, ScriptNode *parent)
{
    ScriptNode *result = nodes.alloc();
    *result = {};
    result->parent = parent;

    if (parent->type == ScriptNode::Type::event)
    {
        ScriptNode *last = parent;
        while (1)
        {
            if (last->next)
            {
                last = last->next;
            }
            else
            {
                break;
            }
        }
        last->next = result;
    }

    result->type = t;
    return result;
}

Script Script::clone()
{
    Script result = {};
    result.nodes =
        FreeList<ScriptNode>::create(Arena::create(&ALLOCATOR_MALLOC));

    AddressFixer<ScriptNode> fixer = AddressFixer<ScriptNode>::create();
    for (auto &a : iter(nodes))
    {
        ScriptNode *b = result.nodes.alloc(a);

        if (&a == root)
        {
            result.root = &a;
            fixer.add_pointer(&result.root);
        }

        fixer.add_mapping(&a, b);
        fixer.add_pointer(&b->parent);
        fixer.add_pointer(&b->next);
        if (b->type == ScriptNode::Type::yesno)
        {
            fixer.add_pointer(&b->yesno.yes);
            fixer.add_pointer(&b->yesno.no);
        }
    }

    fixer.fix_addresses();

    for (auto &a : iter(global_events))
    {
        result.global_events.push(a ? fixer.find_mapping(a) : nullptr);
    }

    fixer.destroy();

    return result;
}

ScriptNode *Script::create_start_event()
{
    ScriptNode *node = nodes.alloc();
    *node = {};
    node->type = ScriptNode::Type::event;
    node->event = ScriptNode::EventType::start;
    global_events.push(node);
    return node;
}

ScriptNode *Script::create_place_event(Place *place)
{
    ScriptNode *node = nodes.alloc();
    *node = {};
    node->type = ScriptNode::Type::event;
    node->event = ScriptNode::EventType::enter;
    node->place = place;
    global_events.push(node);
    return node;
}

ScriptNode *Script::get_start_event()
{
    for (auto &a : iter(global_events))
    {
        if (a->event == ScriptNode::EventType::start)
        {
            return a;
        }
    }
    return nullptr;
}

ScriptNode *Script::get_place_event(Place *place)
{
    for (auto &a : iter(global_events))
    {
        if (a->event == ScriptNode::EventType::enter && a->place == place)
        {
            return a;
        }
    }
    return nullptr;
}

ScriptNode *Script::acquire_start_event()
{
    if (ScriptNode *node = get_start_event())
    {
        return node;
    }
    return create_start_event();
}

ScriptNode *Script::acquire_place_event(Place *place)
{
    if (ScriptNode *node = get_place_event(place))
    {
        return node;
    }
    return create_place_event(place);
}

Script Script::create(Arena arena)
{
    return {FreeList<ScriptNode>::create(arena)};
}

void Script::destroy()
{
    global_events.deinit();
    nodes.destroy();
}
