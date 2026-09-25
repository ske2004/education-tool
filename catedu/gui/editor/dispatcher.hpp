///
/// Dispatcher allows to store history of commands and undo/redo them.
///

#pragma once

#include "catedu/core/alloc/free_list.hpp"
#include "catedu/scene/world.hpp"

struct EditOp
{
    enum class Type
    {
        noop,
        place,
        remove
    };

    Type type;
    Object object;

    // FIXME: Seems to be a hack, but for now I need to store the place as value
    // since it's freed from the free list.
    bool has_place_embedding;
    Place place_embedding;

    Place *place;

    EditOp *next;
    EditOp *prev;
};

struct Dispatcher
{
    World world;
    FreeList<EditOp> history;
    EditOp *current;
    bool dirty;

    static Dispatcher create();
    void destroy();

    void place_object(Object object);
    void remove_object(int x, int y);
    void remove_object(int x, int y, Object::Type type);
    void remove_object(Object *obj);

    void add_script_node(ScriptNode node);

    void enter_place(Object *object);

    void undo();
    void redo();
};
