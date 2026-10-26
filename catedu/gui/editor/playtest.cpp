#include "playtest.hpp"
#include "catedu/core/storage/table.hpp"
#include "catedu/gui/transition/transition.hpp"
#include "catedu/scene/navigation.hpp"
#include "catedu/scene/physics.hpp"
#include "catedu/sys/input.hpp"
#include "catedu/ui/widgets.hpp"
#include "catedu/ui/rendering/make_brush.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>

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

PhysicsWorld create_bodies(Place *parent, Place &place, TableId &player, Table<PlaytestNpc> &npcs)
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
            body.area = {obj.x - 1, obj.y - 1, 2, 2};
            body.solid = true;
            body.dynamic = false;

            physics.bodies.allocate(body);
        }
        break;
        case Object::Type::water: {
            int tx = (int)floorf(obj.x);
            int ty = (int)floorf(obj.y);
            bool covered = place.get_object_at(tx, ty, Object::Type::bridge) != nullptr ||
                           place.get_object_at(tx, ty, Object::Type::road) != nullptr;
            if (!covered)
            {
                PhysicsBody body = {};
                body.area = {obj.x, obj.y, 1, 1};
                body.solid = true;
                body.dynamic = false;

                physics.bodies.allocate(body);
            }
        }
        break;
        case Object::Type::castle:
        case Object::Type::building: {
            PhysicsBody body = {};
            body.area = {obj.x - 4, obj.y - 4, 8, 8};
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
        case Object::Type::animal:
        case Object::Type::npc: {
            PhysicsBody body = {};
            body.area = {obj.x + 0.5f, obj.y + 0.5f, 1, 1};
            body.solid = true;
            body.dynamic = true; // NPCs are dynamic now

            TableId id = physics.bodies.allocate(body);
            PlaytestNpc npc = {id, {0, 0}, 0.0f, &obj};
            npcs.allocate(npc);
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
    Table<PlaytestNpc> npcs = {};
    for (auto &obj : iter(world.first->objects)) {
        if ((obj.type == Object::Type::building || obj.type == Object::Type::castle) && obj.place == nullptr) {
            obj.place = world.places.alloc();
            *obj.place = Place::create();
            obj.place->interior = true;

            int dim = 6 + 2 * obj.floors;
            if (dim > 20) dim = 20;

            int left_x  = -(dim / 2 - 1);
            int right_x = dim / 2;
            int south_y = -12;
            int north_y = south_y + dim - 1;

            for (int x = left_x; x <= right_x; x++) {
                obj.place->place_object({Object::Type::wall, 0, (float)x, (float)south_y});
                obj.place->place_object({Object::Type::wall, 0, (float)x, (float)north_y});
            }
            for (int y = south_y + 1; y <= north_y - 1; y++) {
                obj.place->place_object({Object::Type::wall, 0, (float)left_x, (float)y});
                obj.place->place_object({Object::Type::wall, 0, (float)right_x, (float)y});
            }
            
            int spawn_y = (south_y + north_y) / 2;
            obj.place->place_object({Object::Type::player, 0, 0, (float)spawn_y});
        }
    }

    PhysicsWorld physics = create_bodies(nullptr, *world.first, player, npcs);

    return {player, 0.0f, npcs, physics, world, nullptr, world.script->root, nullptr, {}};
}

void Playtest::destroy()
{
    npcs.deinit();
    physics.bodies.deinit();
    world.destroy();
}

void Playtest::update(UiPass &user, Input &input, EditorCamera &camera,
                      GuiTransition &transition)
{
    // Advance time (1 in-game hour every 5 seconds)
    this->world.time_of_day += (1.0f / (5.0f * 60.0f));
    if (this->world.time_of_day >= 24.0f) this->world.time_of_day -= 24.0f;

    // HUD overlay (time clock and inventory)
    {
        AutoLayoutElement hud = {};
        hud.position = AutoLayoutPosition::absolute;
        hud.offset = {16, 16};
        hud.layout.type = AutoLayout::column;
        hud.padding = {8, 12, 8, 12};
        hud.border = {1, 1, 1, 1};
        hud.width.type = AutoLayoutDimension::autom;
        hud.height.type = AutoLayoutDimension::autom;

        UiBrush bg = UiMakeBrush::make_plain_brush()
                         .squircle(0.75f)
                         .with_solid(0x000000AA)
                         .build();
        UiBrush border = UiMakeBrush::make_plain_brush()
                             .squircle(0.75f)
                             .with_solid(0xFFFFFF33)
                             .build();

        user.begin_generic(hud, bg, border);

        char time_str[64];
        int hours = ((int)floorf(this->world.time_of_day) % 24 + 24) % 24;
        int minutes = ((int)floorf((this->world.time_of_day - floorf(this->world.time_of_day)) * 60.0f) % 60 + 60) % 60;
        snprintf(time_str, sizeof(time_str), "Time: %02d:%02d", hours, minutes);
        label(user, time_str, {1.2f, 1.2f}, UiMakeBrush::make_solid(0xFFFFFFFF));

        for (const auto &item : this->world.player_inventory.items)
        {
            char item_str[128];
            snprintf(item_str, sizeof(item_str), "%s x%d", item.name.c_str(), item.amount);
            label(user, item_str, {1.0f, 1.0f}, UiMakeBrush::make_solid(0xCCCCCCFF));
        }

        user.end_generic();
    }

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
                this->pending = this->current->next;
                this->current = nullptr;
            }
            else
            {
                this->current = this->current->next;
            }
            break;
        case ScriptNode::Type::give_item:
            if (this->current->say[0] != '\0')
            {
                this->world.player_inventory.add_item(this->current->say,
                                                      "Quest reward.", 1);
            }
            this->current = this->current->next;
            break;
        default:
            assert(false);
        }

        while (!this->current && parent && parent->parent &&
               parent->parent->type == ScriptNode::Type::yesno)
        {
            this->current = parent->parent->next;
            parent = parent->parent->parent;
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
    if (input.k[SAPP_KEYCODE_ENTER].pressed)
        enter = true;
        
    if (input.k[SAPP_KEYCODE_SPACE].pressed && player_vz == 0.0f) {
        player_vz = 0.2f;
    }
    
    if (input.k[SAPP_KEYCODE_N].pressed) {
        world.time_of_day = 8.0f;
    }

    movement *= 0.2f;

    if (world.current->interior)
    {
        movement *= -1.0f;
    }

    PhysicsBody &player = physics.bodies.get_assert(this->player);
    Object *under = world.current->get_object_at((int)floorf(player.area.pos.x), (int)floorf(player.area.pos.y));
    if (under && under->type == Object::Type::road)
    {
        movement *= 1.25f;
    }
    else if (under && under->type == Object::Type::high_grass)
    {
        movement *= 0.6f;
    }
    player.area.pos.x += movement.x;
    player.area.pos.y += movement.y;

    std::vector<Object*> items_to_remove;
    for (auto &obj : iter(world.current->objects))
    {
        if (obj.type == Object::Type::item)
        {
            Rect area = {obj.x, obj.y, 1.0f, 1.0f};
            if (rect_vs_rect(player.area, area))
            {
                world.player_inventory.add_item("Found Item", "Picked up from the ground.", 1);
                items_to_remove.push_back(&obj);
            }
        }
    }
    for (auto obj : items_to_remove) {
        world.current->objects.free(obj);
    }

    for (auto [id, npc] : iter(this->npcs))
    {
        PhysicsBody &b = physics.bodies.get_assert(npc.body);
        Vector2 pos = b.area.pos;
        
        if (npc.path_index >= npc.path_count) {
            if (npc.state == NpcState::going_to_work) {
                npc.state = NpcState::working;
                npc.timer = 5.0f; // Work for 5 seconds
            }
            
            if (npc.state == NpcState::working) {
                npc.timer -= 1.0f / 60.0f;
                if (npc.timer <= 0) {
                    // Finished working, drop a coin!
                    Object item = {};
                    item.type = Object::Type::item;
                    item.x = pos.x + (rand() % 3 - 1) * 0.5f;
                    item.y = pos.y + (rand() % 3 - 1) * 0.5f;
                    world.current->objects.alloc(item);
                    
                    npc.state = NpcState::wandering;
                    npc.timer = 2.0f;
                }
            } else if (npc.state == NpcState::wandering) {
                npc.timer -= 1.0f / 60.0f;
                if (npc.timer <= 0) {
                    bool is_day = world.time_of_day >= 8.0f && world.time_of_day <= 18.0f;
                    
                    // Decide what to do: If day: 60% chance to work, 40% to wander
                    if (is_day && npc.obj->type == Object::Type::npc && rand() % 10 < 6) {
                        Object* target_building = nullptr;
                        for (auto& obj : iter(world.current->objects)) {
                            if (obj.type == Object::Type::building) {
                                target_building = &obj;
                                if (rand() % 3 == 0) break; // Pick random building
                            }
                        }
                        
                        if (target_building) {
                            Vector2i start = {(int)roundf(pos.x), (int)roundf(pos.y)};
                            // Pathfind to tile just outside the door
                            Vector2i end = {(int)roundf(target_building->x), (int)roundf(target_building->y - 5)}; 
                            
                            std::vector<Vector2i> path = find_path(world.current, start, end);
                            if (!path.empty()) {
                                npc.path_count = std::min((int)path.size(), 32);
                                npc.path_index = 0;
                                for (int i = 0; i < npc.path_count; i++) {
                                    npc.path[i] = path[i];
                                }
                                npc.state = NpcState::going_to_work;
                            } else {
                                npc.timer = 1.0f;
                            }
                        } else {
                            npc.timer = 1.0f;
                        }
                    } else {
                        // Wander
                        npc.timer = 2.0f + (rand() % 100) / 50.0f;
                        Vector2i start = {(int)roundf(pos.x), (int)roundf(pos.y)};
                        Vector2i end = {start.x + (rand() % 11 - 5), start.y + (rand() % 11 - 5)};
                        
                        if (is_walkable(world.current, end)) {
                            std::vector<Vector2i> path = find_path(world.current, start, end);
                            npc.path_count = std::min((int)path.size(), 32);
                            npc.path_index = 0;
                            for (int i = 0; i < npc.path_count; i++) {
                                npc.path[i] = path[i];
                            }
                        }
                    }
                }
            }
        } else {
            // Move towards current path node
            Vector2 target = {(float)npc.path[npc.path_index].x, (float)npc.path[npc.path_index].y};
            Vector2 diff = {target.x - pos.x, target.y - pos.y};
            float dist = vector2_length(diff);
            
            if (dist < 0.1f) {
                npc.path_index++;
            } else {
                Vector2 move = vector2_normalize(diff) * 0.05f;
                b.area.pos.x += move.x;
                b.area.pos.y += move.y;
            }
        }
    }

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

    if (enter && !switch_target) {
        Object* closest_interactable = nullptr;
        float min_dist = 5.0f; 
        
        for (auto &obj : iter(world.current->objects)) {
            if (obj.type == Object::Type::building || obj.type == Object::Type::tree || obj.type == Object::Type::npc) {
                float dist = vector2_length({obj.x - player.area.pos.x, obj.y - player.area.pos.y});
                if (dist < min_dist) {
                    min_dist = dist;
                    closest_interactable = &obj;
                }
            }
        }
        
        if (closest_interactable) {
            if (closest_interactable->type == Object::Type::tree) {
                world.player_inventory.add_item("Wood", "Chopped from a tree.", 1);
            } else if (closest_interactable->type == Object::Type::building) {
                if (world.player_inventory.remove_item("Wood", 1)) {
                    world.player_inventory.add_item("Coin", "Earned from work.", 5);
                }
            } else if (closest_interactable->type == Object::Type::npc) {
                this->interaction_node = {};
                this->interaction_node.type = ScriptNode::Type::say;
                
                // Random NPC dialogues
                int r = rand() % 3;
                if (r == 0) strcpy(this->interaction_node.say, "Villager: Hello! Nice weather we're having.");
                else if (r == 1) strcpy(this->interaction_node.say, "Villager: I love this town. It's so peaceful.");
                else strcpy(this->interaction_node.say, "Villager: Have you seen the mayor? He looks busy.");
                
                this->current = &this->interaction_node;
            }
        }
    }

    for (auto &obj : iter(world.current->objects))
    {
        if (obj.type == Object::Type::player)
        {
            obj.x = player.area.pos.x;
            obj.y = player.area.pos.y;
            obj.z += player_vz;
            player_vz -= 0.015f; // gravity
            if (obj.z <= 0.0f) {
                obj.z = 0.0f;
                player_vz = 0.0f;
            }

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

    for (auto [id, npc] : iter(this->npcs))
    {
        PhysicsBody &b = physics.bodies.get_assert(npc.body);
        npc.obj->x = b.area.pos.x;
        npc.obj->y = b.area.pos.y;
    }

    if (transition.switching())
    {
        physics.bodies.deinit();
        this->npcs.deinit();
        world.current = switch_target;
        if (this->pending)
        {
            current = this->pending;
            this->pending = nullptr;
        }
        else if (switch_target != world.first)
        {
            current = world.script->acquire_place_event(switch_target);
        }
        this->npcs = {};
        physics = create_bodies(world.first, *world.current, this->player, this->npcs);

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
