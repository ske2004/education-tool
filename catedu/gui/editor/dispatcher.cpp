#include "dispatcher.hpp"
#include "catedu/core/alloc/allocator.hpp"

static void destroy_op_embedding(EditOp &op)
{
    if (op.has_place_embedding)
    {
        op.place_embedding.destroy();
        op.has_place_embedding = false;
        op.place_embedding = {};
    }
}

static void remap_script_place(Script *script, Place *old_place, Place *new_place)
{
    if (!script || !old_place || old_place == new_place)
    {
        return;
    }
    for (auto &node : iter(script->nodes))
    {
        if (node.place == old_place)
        {
            node.place = new_place;
        }
        if (node.type == ScriptNode::Type::teleport &&
            node.teleport.target == old_place)
        {
            node.teleport.target = new_place;
        }
    }
}

void unperform_op(EditOp &op, World *world)
{
    switch (op.type)
    {
    case EditOp::Type::place: {
        Object *obj = world->current->get_object_at(op.object.x, op.object.y,
                                                    op.object.type);
        assert(obj != nullptr);
        if (obj->place && obj->place != world->first)
        {
            destroy_op_embedding(op);
            op.has_place_embedding = true;
            op.place_embedding = obj->place->clone();
            op.object.place = obj->place;
            obj->place->destroy();
            world->places.free(obj->place);
            obj->place = nullptr;
        }
        world->current->remove_object(op.object.x, op.object.y,
                                      op.object.type);
        return;
    }
    case EditOp::Type::remove:
        if (op.has_place_embedding)
        {
            Place *old_place = op.object.place;
            op.object.place = world->places.alloc(op.place_embedding.clone());
            remap_script_place(world->script, old_place, op.object.place);
        }
        {
            Object *placed = world->current->place_object(op.object);
            assert(placed != nullptr);
            (void)placed;
        }
        return;
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
        if (op.has_place_embedding)
        {
            Place *old_place = op.object.place;
            op.object.place = world->places.alloc(op.place_embedding.clone());
            remap_script_place(world->script, old_place, op.object.place);
        }
        return world->current->place_object(op.object) != nullptr;
    case EditOp::Type::remove: {
        Object *obj = world->current->get_object_at(op.object.x, op.object.y,
                                                    op.object.type);
        if (obj == nullptr)
        {
            return false;
        }
        if (obj->place && obj->place != world->first)
        {
            destroy_op_embedding(op);
            op.has_place_embedding = true;
            op.place_embedding = obj->place->clone();
            op.object.place = obj->place;
            obj->place->destroy();
            world->places.free(obj->place);
            obj->place = nullptr;
        }
        world->current->remove_object(op.object.x, op.object.y,
                                      op.object.type);
        return true;
    }
    case EditOp::Type::noop:
        assert(false);
        break;
    }

    assert(false);
    return false;
}

static void append(Dispatcher &disp, EditOp op)
{
    assert(disp.current);
    op.place = disp.world.current;
    if (disp.active_group_id != 0)
    {
        op.group_id = disp.active_group_id;
    }

    EditOp *alloc = disp.history.alloc();
    *alloc = op;

    EditOp *orphan = disp.current->next;
    while (orphan)
    {
        EditOp *next = orphan->next;
        destroy_op_embedding(*orphan);
        disp.history.free(orphan);
        orphan = next;
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
    dispatcher.next_group_id = 1;
    dispatcher.active_group_id = 0;
    dispatcher.group_depth = 0;
    return dispatcher;
}

void Dispatcher::begin_group()
{
    if (active_group_id == 0)
    {
        active_group_id = next_group_id++;
    }
    group_depth++;
}

void Dispatcher::end_group()
{
    if (group_depth > 0)
    {
        group_depth--;
        if (group_depth == 0)
        {
            active_group_id = 0;
        }
    }
    else
    {
        active_group_id = 0;
    }
}

void Dispatcher::destroy()
{
    for (auto &op : iter(history))
    {
        destroy_op_embedding(op);
    }
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
        // Don't remove the old player if the new one can't be placed.
        if (!world.current->can_place_objtype(object.type, object.x, object.y))
        {
            return;
        }

        begin_group();

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
            remove_object(to_remove[i].x, to_remove[i].y, Object::Type::player);
        }
        to_remove.deinit();
    }

    if (perform_op(op, &world))
    {
        dirty = true;
        append(*this, op);
    }

    if (object.type == Object::Type::player)
    {
        end_group();
    }
}

void Dispatcher::remove_object(int x, int y)
{
    remove_object(world.current->get_object_at(x, y));
}

void Dispatcher::remove_object(int x, int y, Object::Type type)
{
    remove_object(world.current->get_object_at(x, y, type));
}

void Dispatcher::remove_object(Object *obj)
{
    if (obj == nullptr)
    {
        return;
    }

    EditOp op = {};
    op.type = EditOp::Type::remove;
    op.object = *obj;
    op.object.x = obj->x;
    op.object.y = obj->y;

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

        Place *place = object->place;

        int dim = 6 + 2 * object->floors;
        if (dim > 20) dim = 20;

        int left_x  = -(dim / 2 - 1);
        int right_x = dim / 2;
        int south_y = -12;
        int north_y = south_y + dim - 1;

        // South wall (solid, exit trigger is at y=-11)
        for (int x = left_x; x <= right_x; x++)
        {
            place->place_object({Object::Type::wall, 0, (float)x, (float)south_y});
        }

        // North wall
        for (int x = left_x; x <= right_x; x++)
        {
            place->place_object({Object::Type::wall, 0, (float)x, (float)north_y});
        }

        // West wall (excluding corners)
        for (int y = south_y + 1; y <= north_y - 1; y++)
        {
            place->place_object({Object::Type::wall, 0, (float)left_x, (float)y});
        }

        // East wall (excluding corners)
        for (int y = south_y + 1; y <= north_y - 1; y++)
        {
            place->place_object({Object::Type::wall, 0, (float)right_x, (float)y});
        }

        // Player spawn point
        int spawn_y = (south_y + north_y) / 2;
        place->place_object({Object::Type::player, 0, 0, (float)spawn_y});
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
    uint32_t gid = current->group_id;
    unperform_op(*current, &world);
    current = current->prev;

    if (gid != 0)
    {
        while (current && current->prev && current->place == world.current &&
               current->group_id == gid)
        {
            unperform_op(*current, &world);
            current = current->prev;
        }
    }
}

void Dispatcher::redo()
{
    if (!current || !current->next || current->next->place != world.current)
    {
        return;
    }

    uint32_t gid = current->next->group_id;

    current = current->next;
    dirty = true;
    bool ok = perform_op(*current, &world);
    assert(ok);
    (void)ok;

    if (gid != 0)
    {
        while (current->next && current->next->place == world.current &&
               current->next->group_id == gid)
        {
            current = current->next;
            bool ok = perform_op(*current, &world);
            assert(ok);
            (void)ok;
        }
    }
}
