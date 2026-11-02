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

static Object *place_with_id(Place *place, Object::Type type, float x, float y,
                             const char *id)
{
    Object obj = {type, 0, x, y};
    if (id)
    {
        strncpy(obj.id, id, sizeof(obj.id) - 1);
        obj.id[sizeof(obj.id) - 1] = '\0';
    }
    Object *placed = place->place_object(obj);
    if (placed && id)
    {
        strncpy(placed->id, id, sizeof(placed->id) - 1);
        placed->id[sizeof(placed->id) - 1] = '\0';
    }
    return placed;
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
    place_with_id(town_hall_interior, Object::Type::npc, 0, -6, "Mayor");
    // Town Hall civic furnishings
    place_with_id(town_hall_interior, Object::Type::prop, 0, -2, "counter");
    place_with_id(town_hall_interior, Object::Type::prop, -3, -4, "bench");
    place_with_id(town_hall_interior, Object::Type::prop, 3, -4, "bench");
    place_with_id(town_hall_interior, Object::Type::prop, -5, 1, "bookshelf");
    place_with_id(town_hall_interior, Object::Type::prop, 5, 1, "bookshelf");
    place_with_id(town_hall_interior, Object::Type::prop, -5, -9, "lantern");
    place_with_id(town_hall_interior, Object::Type::prop, 5, -9, "lantern");

    // Warehouse / Engineering Workshop (west, 3 floors)
    Object *warehouse = outdoor->place_object(
        {Object::Type::building, 3, -14, 0});
    generate_interior(warehouse, disp.world);
    Place *warehouse_interior = warehouse->place;
    // Partitions and workbenches/storage props
    warehouse_interior->place_object({Object::Type::wall, 0, -2, -10});
    warehouse_interior->place_object({Object::Type::wall, 0, 2, -8});
    warehouse_interior->place_object({Object::Type::wall, 0, -2, -7});
    place_with_id(warehouse_interior, Object::Type::prop, -4, -3, "barrel");
    place_with_id(warehouse_interior, Object::Type::prop, -3, -3, "barrel");
    place_with_id(warehouse_interior, Object::Type::prop, 4, -3, "crate");
    place_with_id(warehouse_interior, Object::Type::prop, 2, -3, "counter");
    place_with_id(warehouse_interior, Object::Type::prop, 0, -3, "lantern");

    // House / Astronomy Library (north, 2 floors)
    Object *house = outdoor->place_object(
        {Object::Type::building, 2, 0, 18});
    generate_interior(house, disp.world);
    Place *house_interior = house->place;
    // Study partitions and library furnishings
    house_interior->place_object({Object::Type::wall, 0, -1, -10});
    house_interior->place_object({Object::Type::wall, 0, 2, -9});
    place_with_id(house_interior, Object::Type::prop, -3, -4, "bookshelf");
    place_with_id(house_interior, Object::Type::prop, 3, -4, "bookshelf");
    place_with_id(house_interior, Object::Type::prop, 1, -5, "bench");
    place_with_id(house_interior, Object::Type::prop, -2, -5, "lantern");
    place_with_id(house_interior, Object::Type::prop, 3, -10, "flower_pot");

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
    for (int gx = 8; gx <= 12; gx += 2)
    {
        for (int gy = 8; gy <= 12; gy += 2)
        {
            outdoor->place_object(
                {Object::Type::high_grass, 0, (float)gx, (float)gy});
        }
    }
    for (int gx = -12; gx <= -8; gx += 2)
    {
        for (int gy = 8; gy <= 12; gy += 2)
        {
            outdoor->place_object(
                {Object::Type::high_grass, 0, (float)gx, (float)gy});
        }
    }

    // === River ===
    // A river crossing the southern road
    for (int x = -14; x <= 14; x += 2)
    {
        if (x != 0 && x != -2 && x != 2)
        { // Bridge gap over the road
            outdoor->place_object({Object::Type::water, 0, (float)x, -16});
        }
    }
    // Bridge (road)
    outdoor->place_object({Object::Type::road, 0, 0, -16});

    // === Animals ===
    outdoor->place_object({Object::Type::animal, 0, 9, 9});
    outdoor->place_object({Object::Type::animal, 0, -9, 9});
    outdoor->place_object({Object::Type::animal, 0, -11, 11});

    // === NPCs ===
    place_with_id(outdoor, Object::Type::npc, 4, 4, "Engineer");
    place_with_id(outdoor, Object::Type::npc, -4, 4, "Botanist");
    place_with_id(outdoor, Object::Type::npc, 4, -4, "Librarian");

    // === Educational Quest Items ===
    place_with_id(outdoor, Object::Type::item, 9, 8, "Solar Lens");
    place_with_id(outdoor, Object::Type::item, -8, -8, "Ancient Gear");

    // === Outdoor Plaza & Street Props ===
    place_with_id(outdoor, Object::Type::prop, 3, -3, "fountain");
    place_with_id(outdoor, Object::Type::prop, -3, -3, "bench");
    place_with_id(outdoor, Object::Type::prop, 3, 3, "bench");
    place_with_id(outdoor, Object::Type::prop, -3, 3, "lantern");
    place_with_id(outdoor, Object::Type::prop, 9, -3, "flower_pot");
    place_with_id(outdoor, Object::Type::prop, -9, -3, "barrel");

    // === Player ===
    outdoor->place_object({Object::Type::player, 0, 0, -10});

    // === Scripts ===
    Script *script = disp.world.script;

    // Start event: welcome message introducing Greenville's renewable energy quest
    ScriptNode *start_event = script->create_start_event();
    ScriptNode *welcome = script->append_node(ScriptNode::Type::say, start_event);
    strcpy(welcome->say,
        "Welcome to Greenville! Our town runs on renewable solar energy, focusing sunlight with a Piezo-Quartz Crystal. Visit the Mayor in Town Hall to learn more!");
    script->root = start_event;

    // Town Hall enter event: civic & solar concentrator lesson
    ScriptNode *th_event = script->create_place_event(town_hall_interior);
    ScriptNode *th_say = script->append_node(ScriptNode::Type::say, th_event);
    strcpy(th_say->say,
        "Mayor: Our Piezo-Quartz Crystal is in the west Workshop! Solar concentrators focus sunlight to generate clean electricity. Please retrieve the crystal for our grid!");

    // Warehouse enter event: yesno with gear & piezoelectric mechanics
    ScriptNode *wh_event = script->create_place_event(warehouse_interior);
    ScriptNode *wh_yesno = script->append_node(ScriptNode::Type::yesno, wh_event);
    strcpy(wh_yesno->yesno.question,
        "You found the Piezo-Quartz Crystal! Its lattice converts mechanical gear torque and focused solar heat into voltage. Return it to the Mayor at Town Hall?");

    // Yes branch: give crystal, teleport to Town Hall, then thank-you message
    ScriptNode *yes_event = script->append_node(ScriptNode::Type::event, wh_yesno);
    yes_event->event = ScriptNode::EventType::yes;
    wh_yesno->yesno.yes = yes_event;

    ScriptNode *give = script->append_node(ScriptNode::Type::give_item, yes_event);
    strcpy(give->say, "Crystal");

    ScriptNode *tp = script->append_node(ScriptNode::Type::teleport, yes_event);
    tp->teleport.target = town_hall_interior;

    ScriptNode *thanks = script->append_node(ScriptNode::Type::say, yes_event);
    strcpy(thanks->say,
        "Mayor: Thank you! With the crystal and compound gears multiplying torque, Greenville's renewable microgrid is back at 100% clean power!");

    // No branch: gear ratio mechanics observation
    ScriptNode *no_event = script->append_node(ScriptNode::Type::event, wh_yesno);
    no_event->event = ScriptNode::EventType::no;
    wh_yesno->yesno.no = no_event;

    ScriptNode *no_say = script->append_node(ScriptNode::Type::say, no_event);
    strcpy(no_say->say,
        "You leave the crystal on the workbench to study how step-up gear ratios trade torque for rotational speed...");

    // House / Library enter event: solar astronomy lesson
    ScriptNode *house_event = script->create_place_event(house_interior);
    ScriptNode *house_say = script->append_node(ScriptNode::Type::say, house_event);
    strcpy(house_say->say,
        "Library Notes: Solar astronomy shows Earth's 23.5-degree axial tilt causes seasons. Tracking the sun's azimuth maximizes solar lens efficiency all year!");

    disp.dirty = true;
    return disp;
}
