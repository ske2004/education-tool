#include "dispatcher.hpp"
#include "catedu/core/alloc/allocator.hpp"

void unperform_op(EditOp &op, World *world)
{
    switch (op.type)
    {
    case EditOp::Type::place:
        assert(world->current->get_object_at(op.object.x, op.object.y) !=
               nullptr);
        world->current->remove_object(op.object.x, op.object.y);
        return;
        break;
    case EditOp::Type::remove:
        if (op.has_place_embedding)
        {
            op.object.place = world->places.alloc(op.place_embedding);
        }
        {
            Object *placed = world->current->place_object(op.object);
            assert(placed != nullptr);
            (void)placed;
        }
        return;
        break;
    case EditOp::Type::noop:
        assert(false);
        break;
    }

    assert(false);
}

bool perform_op(EditOp &op, World *world)
{
    switch (op.type)
    {
    case EditOp::Type::place:
        return world->current->place_object(op.object) != nullptr;
        break;
    case EditOp::Type::remove:
        if (world->current->get_object_at(op.object.x, op.object.y) == nullptr)
        {
            return false;
        }
        world->current->remove_object(op.object.x, op.object.y);
        return true;
        break;
    case EditOp::Type::noop:
        assert(false);
        break;
        break;
    }

    assert(false);
    return false;
}

static void append(Dispatcher &disp, EditOp op)
{
    assert(disp.current);
    op.place = disp.world.current;

    EditOp *alloc = disp.history.alloc();
    *alloc = op;

    EditOp *orphan = disp.current->next;
    while (orphan)
    {
        disp.history.free(orphan);
        orphan = orphan->next;
    }

    disp.current->next = alloc;
    alloc->prev = disp.current;
    disp.current = alloc;
}

Dispatcher Dispatcher::create()
{
    Dispatcher dispatcher = {};
    dispatcher.history =
        FreeList<EditOp>::create(Arena::create(&ALLOCATOR_MALLOC));
    dispatcher.current = dispatcher.history.alloc();
    *dispatcher.current = {};
    dispatcher.current->type = EditOp::Type::noop;
    dispatcher.world = World::create();
    return dispatcher;
}

void Dispatcher::destroy()
{
    world.destroy();
    history.destroy();
}

void Dispatcher::place_object(Object object)
{
    EditOp op = {};
    op.type = EditOp::Type::place;
    op.object = object;

    if (object.type == Object::Type::player)
    {
        struct Pos { int x, y; };
        Stack<Pos> to_remove = {};
        for (auto &obj : iter(world.current->objects))
        {
            if (obj.type == Object::Type::player)
            {
                to_remove.push({(int)obj.x, (int)obj.y});
            }
        }
        for (size_t i = 0; i < to_remove.count; i++)
        {
            remove_object(to_remove[i].x, to_remove[i].y);
        }
        to_remove.deinit();
    }

    if (perform_op(op, &world))
    {
        dirty = true;
        append(*this, op);
    }
}

void Dispatcher::remove_object(int x, int y)
{
    Object *obj = world.current->get_object_at(x, y);
    Place *place_embedding = nullptr;

    if (obj == nullptr)
    {
        return;
    }

    if (obj->place && obj->place != world.first)
    {
        place_embedding = obj->place;
    }

    obj->place = nullptr;

    EditOp op = {};
    op.type = EditOp::Type::remove;
    op.object = *obj;
    op.object.x = obj->x;
    op.object.y = obj->y;

    if (place_embedding)
    {
        op.has_place_embedding = true;
        op.place_embedding = place_embedding->clone();
        world.places.free(place_embedding);
    }

    if (perform_op(op, &world))
    {
        dirty = true;
        append(*this, op);
    }
}

void Dispatcher::add_script_node(ScriptNode node)
{
}

void Dispatcher::enter_place(Object *object)
{
    if (object->place == nullptr)
    {
        object->place = world.places.alloc();
        *object->place = Place::create();
        object->place->interior = true;
    }

    world.current = object->place;
}

void Dispatcher::undo()
{
    if (!current || !current->prev || current->place != world.current)
    {
        return;
    }

    dirty = true;
    unperform_op(*current, &world);

    current = current->prev;
}

void Dispatcher::redo()
{
    if (!current || !current->next || current->place != world.current)
    {
        return;
    }

    current = current->next;

    dirty = true;
    bool ok = perform_op(*current, &world);
    assert(ok);
    (void)ok;
}
