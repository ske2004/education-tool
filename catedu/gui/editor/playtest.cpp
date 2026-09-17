#include "playtest.hpp"
#include "catedu/core/storage/table.hpp"
#include "catedu/gui/transition/transition.hpp"
#include "catedu/scene/physics.hpp"
#include "catedu/sys/input.hpp"
#include "catedu/ui/widgets.hpp"
#include <cstdio>

int show_say(UiPass &user, const char *msg)
{
    const char *btns[] = {"Next", NULL};
    switch (msgbox(user, "Dialog", msg, MsgBoxType::Info, btns))
    {
    case 0:
        return 0;
        break;
    }
    return -1;
}

int show_yesno(UiPass &user, const char *msg)
{
    const char *btns[] = {"Yes", "No", NULL};
    switch (msgbox(user, "Dialog", msg, MsgBoxType::Info, btns))
    {
    case 0:
        return 0;
        break;
    case 1:
        return 1;
        break;
    }
    return -1;
}

PhysicsWorld create_bodies(Place *parent, Place &place, TableId &player)
{
    PhysicsWorld physics = {};
    player = NULL_ID;
    for (auto &obj : iter(place.objects))
    {
        switch (obj.type)
        {
        case Object::Type::wall: {
            PhysicsBody body = {};
            body.area = {obj.x + 0.5f, obj.y + 0.5f, 1, 1};
            body.solid = true;
            body.dynamic = false;

            physics.bodies.allocate(body);
        }
        break;
        case Object::Type::tree: {
            PhysicsBody body = {};
            body.area = {obj.x, obj.y, 1, 1};
            body.solid = true;
            body.dynamic = false;

            physics.bodies.allocate(body);
        }
        break;
        case Object::Type::building: {
            PhysicsBody body = {};
            body.area = {obj.x - 4 + 0.5f, obj.y - 4 + 0.5f, 8, 8};
            body.solid = true;
            body.dynamic = false;

            physics.bodies.allocate(body);

            PhysicsBody door = {};
            door.area = {obj.x, obj.y - 4.1f, 1, 1};
            door.solid = false;
            door.dynamic = false;
            door.userdata = obj.place;

            physics.bodies.allocate(door);
        }
        break;
        case Object::Type::player: {
            PhysicsBody body = {};
            body.area = {obj.x, obj.y, 1, 1};
            body.solid = true;
            body.dynamic = true;

            player = physics.bodies.allocate(body);
        }
        break;
        default:
            break;
        }
    }

    if (place.interior)
    {
        PhysicsBody door = {};
        door.area = {0, -11, 2, 1};
        door.solid = false;
        door.dynamic = false;
        door.userdata = parent;

        physics.bodies.allocate(door);
    }

    return physics;
}

Playtest Playtest::create(World world)
{
    TableId player = {};
    PhysicsWorld physics = create_bodies(nullptr, *world.first, player);
    return {player, physics, world, nullptr, world.script->root};
}

void Playtest::destroy()
{
    physics.bodies.deinit();
    world.destroy();
}

void Playtest::update(UiPass &user, Input &input, EditorCamera &camera,
                      GuiTransition &transition)
{
    if (this->current)
    {
        ScriptNode *parent = this->current->parent;

        switch (this->current->type)
        {
        case ScriptNode::Type::event:
            this->current = this->current->next;
            break;
        case ScriptNode::Type::say:
            if (show_say(user, this->current->say) == 0)
            {
                this->current = this->current->next;
            }
            break;
        case ScriptNode::Type::yesno:
            switch (show_yesno(user, this->current->yesno.question))
            {
            case 0:
                this->current = this->current->yesno.yes;
                break;
            case 1:
                this->current = this->current->yesno.no;
                break;
            }
            break;
        case ScriptNode::Type::teleport:
            if (this->current->teleport.target)
            {
                switch_target = this->current->teleport.target;
                transition.begin();
                this->current = nullptr;
            }
            else
            {
                this->current = this->current->next;
            }
            break;
        default:
            assert(false);
        }

        if (!this->current && parent && parent->parent &&
            parent->parent->type == ScriptNode::Type::yesno)
        {
            this->current = parent->parent->next;
        }

        return;
    }

    if (this->player == NULL_ID)
    {
        return;
    }

    bool enter = false;
    Vector2 movement = {0, 0};

    if (input.k[SAPP_KEYCODE_UP].held)
        movement.y += 1;
    if (input.k[SAPP_KEYCODE_DOWN].held)
        movement.y -= 1;
    if (input.k[SAPP_KEYCODE_LEFT].held)
        movement.x -= 1;
    if (input.k[SAPP_KEYCODE_RIGHT].held)
        movement.x += 1;
    if (input.k[SAPP_KEYCODE_SPACE].pressed)
        enter = true;

    movement *= 0.2f;

    if (world.current->interior)
    {
        movement *= -1.0f;
    }

    PhysicsBody &player = physics.bodies.get_assert(this->player);
    Object *under = world.current->get_object_at((int)floorf(player.area.pos.x), (int)floorf(player.area.pos.y));
    if (under && under->type == Object::Type::road)
    {
        movement *= 2.0f;
    }
    player.area.pos.x += movement.x;
    player.area.pos.y += movement.y;

    PhysicsManifolds manifolds = physics.detect_collisions();
    physics.resolve_physics(manifolds);
    for (auto [id, manifold] : iter(manifolds.manifolds))
    {
        PhysicsBody &b = physics.bodies.get_assert(manifold.second);

        if (manifold.first == this->player && b.userdata)
        {
            if (enter)
            {
                void *userdata = b.userdata;
                switch_target = (Place *)userdata;
                transition.begin();
                break;
            }
        }
    }
    manifolds.manifolds.deinit();

    player = physics.bodies.get_assert(this->player);

    for (auto &obj : iter(world.current->objects))
    {
        if (obj.type == Object::Type::player)
        {
            obj.x = player.area.pos.x;
            obj.y = player.area.pos.y;

            if (world.current->interior)
            {
                camera.follow({obj.x, 0, obj.y}, 180.0f);
            }
            else
            {
                camera.follow({obj.x, 0, obj.y}, 0);
            }
        }
    }

    if (transition.switching())
    {
        physics.bodies.deinit();
        world.current = switch_target;
        if (switch_target != world.first)
        {
            current = world.script->acquire_place_event(switch_target);
        }
        physics = create_bodies(world.first, *world.current, this->player);

        if (this->player == NULL_ID)
        {
            return;
        }

        PhysicsBody &player = physics.bodies.get_assert(this->player);

        camera.lockin({player.area.pos.x, 0, player.area.pos.y},
                      world.current->interior ? 180.0f : 0);
        switch_target = nullptr;
    }
}
