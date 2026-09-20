#include "sample_world.hpp"
#include <cstring>

static void generate_interior(Object *object, World &world)
{
    object->place = world.places.alloc();
    *object->place = Place::create();
    object->place->interior = true;

    Place *place = object->place;

    int dim = 6 + 2 * object->floors;
    if (dim > 20)
        dim = 20;

    int left_x = -(dim / 2 - 1);
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

    // Player spawn at center
    int spawn_y = (south_y + north_y) / 2;
    place->place_object({Object::Type::player, 0, 0, (float)spawn_y});
}

Dispatcher generate_sample_world()
{
    Dispatcher disp = Dispatcher::create();
    Place *outdoor = disp.world.first;
    disp.world.current = outdoor;

    // === Roads: vertical arm ===
    outdoor->place_object({Object::Type::road, 0, 0, 0});
    outdoor->place_object({Object::Type::road, 0, 0, 4});
    outdoor->place_object({Object::Type::road, 0, 0, 8});
    outdoor->place_object({Object::Type::road, 0, 0, 12});
    outdoor->place_object({Object::Type::road, 0, 0, -4});
    outdoor->place_object({Object::Type::road, 0, 0, -8});

    // === Roads: horizontal arm ===
    outdoor->place_object({Object::Type::road, 0, 4, 0});
    outdoor->place_object({Object::Type::road, 0, 8, 0});
    outdoor->place_object({Object::Type::road, 0, -4, 0});
    outdoor->place_object({Object::Type::road, 0, -8, 0});

    // === Buildings ===

    // Town Hall (east, 5 floors)
    Object *town_hall = outdoor->place_object(
        {Object::Type::building, 5, 14, 0});
    generate_interior(town_hall, disp.world);
    Place *town_hall_interior = town_hall->place;
    // Mayor NPC near center-north of room
    town_hall_interior->place_object({Object::Type::npc, 0, 0, -6});

    // Warehouse (west, 3 floors)
    Object *warehouse = outdoor->place_object(
        {Object::Type::building, 3, -14, 0});
    generate_interior(warehouse, disp.world);
    Place *warehouse_interior = warehouse->place;
    // Crates/shelves (wall objects)
    warehouse_interior->place_object({Object::Type::wall, 0, -2, -10});
    warehouse_interior->place_object({Object::Type::wall, 0, 2, -8});
    warehouse_interior->place_object({Object::Type::wall, 0, -1, -7});

    // House (north, 2 floors)
    Object *house = outdoor->place_object(
        {Object::Type::building, 2, 0, 18});
    generate_interior(house, disp.world);
    Place *house_interior = house->place;
    // Furniture (wall objects as table and chair stand-ins)
    house_interior->place_object({Object::Type::wall, 0, -1, -10});
    house_interior->place_object({Object::Type::wall, 0, 2, -9});

    // === Trees and Grass ===
    outdoor->place_object({Object::Type::tree, 0, 6, 10});
    outdoor->place_object({Object::Type::tree, 0, -6, 10});
    outdoor->place_object({Object::Type::tree, 0, 10, 6});
    outdoor->place_object({Object::Type::tree, 0, -10, 6});
    outdoor->place_object({Object::Type::tree, 0, 10, -6});
    outdoor->place_object({Object::Type::tree, 0, -10, -6});
    outdoor->place_object({Object::Type::tree, 0, 6, -10});
    outdoor->place_object({Object::Type::tree, 0, -6, -10});
    
    // High grass patches
    for (int gx = 8; gx <= 12; gx+=2) {
        for (int gy = 8; gy <= 12; gy+=2) {
            outdoor->place_object({Object::Type::high_grass, 0, (float)gx, (float)gy});
        }
    }
    for (int gx = -12; gx <= -8; gx+=2) {
        for (int gy = 8; gy <= 12; gy+=2) {
            outdoor->place_object({Object::Type::high_grass, 0, (float)gx, (float)gy});
        }
    }

    // === River ===
    // A river crossing the southern road
    for (int x = -14; x <= 14; x+=2) {
        if (x != 0 && x != -2 && x != 2) { // Bridge gap over the road
            outdoor->place_object({Object::Type::water, 0, (float)x, -16});
        }
    }
    // Bridge (road)
    outdoor->place_object({Object::Type::road, 0, 0, -16});
    
    // === Animals ===
    outdoor->place_object({Object::Type::animal, 0, 10, 10});
    outdoor->place_object({Object::Type::animal, 0, -10, 10});
    outdoor->place_object({Object::Type::animal, 0, -12, 12});
    
    // === NPCs ===
    outdoor->place_object({Object::Type::npc, 0, 4, 4});
    outdoor->place_object({Object::Type::npc, 0, -4, 4});
    outdoor->place_object({Object::Type::npc, 0, 4, -4});
    
    // === Items ===
    outdoor->place_object({Object::Type::item, 0, 10, 8});
    outdoor->place_object({Object::Type::item, 0, -8, -8});

    // === Player ===
    outdoor->place_object({Object::Type::player, 0, 0, -10});

    // === Scripts ===
    Script *script = disp.world.script;

    // Start event: welcome message
    ScriptNode *start_event = script->create_start_event();
    ScriptNode *welcome = script->append_node(ScriptNode::Type::say, start_event);
    strcpy(welcome->say,
        "Welcome to Greenville! Talk to the Mayor inside the Town Hall.");
    script->root = start_event;

    // Town Hall enter event
    ScriptNode *th_event = script->create_place_event(town_hall_interior);
    ScriptNode *th_say = script->append_node(ScriptNode::Type::say, th_event);
    strcpy(th_say->say,
        "Mayor: The town crystal was stolen! Find it in the old warehouse to the west.");

    // Warehouse enter event: yesno
    ScriptNode *wh_event = script->create_place_event(warehouse_interior);
    ScriptNode *wh_yesno = script->append_node(ScriptNode::Type::yesno, wh_event);
    strcpy(wh_yesno->yesno.question,
        "You found the crystal! Return it to the Mayor?");

    // Yes branch: teleport to Town Hall, then thank-you message
    ScriptNode *yes_event = script->append_node(ScriptNode::Type::event, wh_yesno);
    yes_event->event = ScriptNode::EventType::yes;
    wh_yesno->yesno.yes = yes_event;

    ScriptNode *tp = script->append_node(ScriptNode::Type::teleport, yes_event);
    tp->teleport.target = town_hall_interior;

    ScriptNode *thanks = script->append_node(ScriptNode::Type::say, yes_event);
    strcpy(thanks->say, "Mayor: Thank you! Greenville is saved!");

    // No branch: flavor text
    ScriptNode *no_event = script->append_node(ScriptNode::Type::event, wh_yesno);
    no_event->event = ScriptNode::EventType::no;
    wh_yesno->yesno.no = no_event;

    ScriptNode *no_say = script->append_node(ScriptNode::Type::say, no_event);
    strcpy(no_say->say, "You leave the crystal where it is...");

    // House enter event
    ScriptNode *house_event = script->create_place_event(house_interior);
    ScriptNode *house_say = script->append_node(ScriptNode::Type::say, house_event);
    strcpy(house_say->say,
        "This is a quiet home. Nobody seems to be around.");

    disp.dirty = true;
    return disp;
}
