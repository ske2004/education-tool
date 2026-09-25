#include "world.hpp"
#include "catedu/core/alloc/allocator.hpp"
#include "catedu/core/memory/addressfixer.hpp"
#define BUILDING_DIMENSIONS_W 8
#define BUILDING_DIMENSIONS_D 8

RectI object_dimensions(Object &object)
{
    switch (object.type)
    {
    case Object::Type::building:
        return {(int)ceilf(object.x) - BUILDING_DIMENSIONS_W / 2,
                (int)ceilf(object.y) - BUILDING_DIMENSIONS_D / 2,
                BUILDING_DIMENSIONS_W, BUILDING_DIMENSIONS_D};
    case Object::Type::road:
        return {(int)ceilf(object.x) - 2, (int)ceilf(object.y) - 2, 4, 4};
    case Object::Type::player:
        return {(int)ceilf(object.x) - 1, (int)ceilf(object.y) - 1, 2, 2};
    case Object::Type::wall:
        return {(int)ceilf(object.x), (int)ceilf(object.y), 1, 1};
    case Object::Type::tree:
        return {(int)ceilf(object.x) - 1, (int)ceilf(object.y) - 1, 2, 2};
    case Object::Type::npc:
        return {(int)ceilf(object.x), (int)ceilf(object.y), 1, 1};
    case Object::Type::item:
        return {(int)ceilf(object.x), (int)ceilf(object.y), 1, 1};
    case Object::Type::animal:
        return {(int)ceilf(object.x), (int)ceilf(object.y), 1, 1};
    case Object::Type::water:
        return {(int)ceilf(object.x), (int)ceilf(object.y), 1, 1};
    case Object::Type::high_grass:
        return {(int)ceilf(object.x), (int)ceilf(object.y), 1, 1};
    case Object::Type::prop:
        return {(int)ceilf(object.x), (int)ceilf(object.y), 1, 1};
    case Object::Type::bridge:
        return {(int)ceilf(object.x), (int)ceilf(object.y), 1, 1};
    case Object::Type::castle:
        return {(int)ceilf(object.x) - BUILDING_DIMENSIONS_W / 2,
                (int)ceilf(object.y) - BUILDING_DIMENSIONS_D / 2,
                BUILDING_DIMENSIONS_W, BUILDING_DIMENSIONS_D};
    }

    assert(false);
}

bool point_in_dimensions(int x, int y, RectI dimensions)
{
    return dimensions.pos.x <= x && dimensions.pos.x + dimensions.siz.x > x &&
           dimensions.pos.y <= y && dimensions.pos.y + dimensions.siz.y > y;
}

// The player is a spawn marker that doesn't claim space, so it can share
// tiles with roads. Lookups prefer the object the player is standing on.
Object *object_space(int x, int y, FreeList<Object> &objects)
{
    Object *player = nullptr;
    for (auto &object : iter(objects))
    {
        if (point_in_dimensions(x, y, object_dimensions(object)))
        {
            if (object.type != Object::Type::player)
            {
                return &object;
            }
            player = &object;
        }
    }

    return player;
}

Object *object_space(int x, int y, Object::Type type, FreeList<Object> &objects)
{
    for (auto &object : iter(objects))
    {
        if (object.type == type &&
            point_in_dimensions(x, y, object_dimensions(object)))
        {
            return &object;
        }
    }

    return nullptr;
}

static bool rects_overlap(RectI a, RectI b)
{
    return a.pos.x < b.pos.x + b.siz.x && b.pos.x < a.pos.x + a.siz.x &&
           a.pos.y < b.pos.y + b.siz.y && b.pos.y < a.pos.y + a.siz.y;
}

static bool region_overlaps_player(RectI region, FreeList<Object> &objects)
{
    for (auto &object : iter(objects))
    {
        if (object.type == Object::Type::player &&
            rects_overlap(region, object_dimensions(object)))
        {
            return true;
        }
    }

    return false;
}

Place Place::create()
{
    Place place = {};

    place.space = Space::create();
    place.objects = FreeList<Object>::create(Arena::create(&ALLOCATOR_MALLOC));

    return place;
}

void Place::destroy()
{
    objects.destroy();
    space.destroy();
}

Place Place::clone()
{
    Place world = Place::create();
    world.interior = this->interior;

    for (auto &object : iter(objects))
    {
        if (object.type != Object::Type::player)
        {
            world.space.claim_region_rect(object_dimensions(object));
        }
        Object *obj = world.objects.alloc();
        *obj = object;
    }

    return world;
}

bool Place::can_place_building(int floors, int x, int y)
{
    RectI region = {x - 4, y - 4, 8, 8};

    return !space.is_region_claimed(region) &&
           !region_overlaps_player(region, objects);
}

static bool can_place_region(Place &place, Object::Type type, RectI region)
{
    switch (type)
    {
    case Object::Type::player:
        // Every tile must be free or covered by a road.
        for (int ty = region.pos.y; ty < region.pos.y + region.siz.y; ty++)
        {
            for (int tx = region.pos.x; tx < region.pos.x + region.siz.x; tx++)
            {
                if (place.space.is_region_claimed({tx, ty, 1, 1}) &&
                    object_space(tx, ty, Object::Type::road, place.objects) ==
                        nullptr)
                {
                    return false;
                }
            }
        }
        return true;
    case Object::Type::road:
        return !place.space.is_region_claimed(region);
    default:
        return !place.space.is_region_claimed(region) &&
               !region_overlaps_player(region, place.objects);
    }
}

bool Place::can_place_objtype(Object::Type type, int x, int y)
{
    Object mock = {};
    mock.type = type;
    mock.x = x;
    mock.y = y;
    return can_place_region(*this, type, object_bounds(mock));
}

Object *Place::place_object(Object object)
{
    if (!can_place_region(*this, object.type, object_dimensions(object)))
    {
        return nullptr;
    }

    if (object.type != Object::Type::player)
    {
        space.claim_region_rect(object_dimensions(object));
    }

    Object *obj = objects.alloc();
    *obj = object;

    return obj;
}

static void remove_object_ptr(Place &place, Object *obj)
{
    if (obj->type != Object::Type::player)
    {
        place.space.unclaim_region_rect(object_dimensions(*obj));
    }
    place.objects.free(obj);
}

void Place::remove_object(int x, int y)
{
    if (Object *obj = object_space(x, y, objects); obj != nullptr)
    {
        remove_object_ptr(*this, obj);
    }
}

void Place::remove_object(int x, int y, Object::Type type)
{
    if (Object *obj = object_space(x, y, type, objects); obj != nullptr)
    {
        remove_object_ptr(*this, obj);
    }
}

Object *Place::get_object_at(int x, int y)
{
    return object_space(x, y, objects);
}

Object *Place::get_object_at(int x, int y, Object::Type type)
{
    return object_space(x, y, type, objects);
}

RectI Place::object_bounds(Object &object)
{
    return object_dimensions(object);
}

World World::create()
{
    World world = {};
    world.places = FreeList<Place>::create(Arena::create(&ALLOCATOR_MALLOC));
    world.first = world.places.alloc();
    *world.first = Place::create();
    world.current = world.first;

    // Very bad, I need the pointer because it will escape if it's on stack.
    world.script = (Script *)ALLOCATOR_MALLOC.alloc(sizeof(Script));
    *world.script = Script::create(Arena::create(&ALLOCATOR_MALLOC));

    world.time_of_day = 12.0f;
    world.player_inventory.capacity = 16;
    return world;
}

void World::destroy()
{
    for (auto &place : iter(places))
    {
        place.destroy();
    }
    script->destroy();
    ALLOCATOR_MALLOC.free(script);
    places.destroy();

    first = nullptr;
}

World World::clone()
{
    World world = World::create();

    world.script->destroy();
    ALLOCATOR_MALLOC.free(world.script);
    world.script = (Script *)ALLOCATOR_MALLOC.alloc(sizeof(Script));
    *world.script = script->clone();

    world.time_of_day = this->time_of_day;
    world.player_inventory = this->player_inventory;

    AddressFixer<Place> fixer = AddressFixer<Place>::create();

    for (auto &place : iter(places))
    {
        Place *new_place = world.places.alloc();
        *new_place = place.clone();
        fixer.add_mapping(&place, new_place);

        if (&place == first)
        {
            world.first = new_place;
            world.current = new_place;
        }
    }

    for (auto &place : iter(world.places))
    {
        for (auto &object : iter(place.objects))
        {
            if (object.place)
            {
                fixer.add_pointer(&object.place);
            }
        }
    }

    // Fix Place pointers inside script nodes (enter event targets, teleport targets)
    for (auto &node : iter(world.script->nodes))
    {
        if (node.place)
        {
            fixer.add_pointer(&node.place);
        }
        if (node.type == ScriptNode::Type::teleport && node.teleport.target)
        {
            fixer.add_pointer(&node.teleport.target);
        }
    }

    fixer.fix_addresses();
    fixer.destroy();

    return world;
}
