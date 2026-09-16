#include "world.hpp"
#include "catedu/core/alloc/allocator.hpp"
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
    }

    assert(false);
}

bool point_in_dimensions(int x, int y, RectI dimensions)
{
    return dimensions.pos.x <= x && dimensions.pos.x + dimensions.siz.x > x &&
           dimensions.pos.y <= y && dimensions.pos.y + dimensions.siz.y > y;
}

Object *object_space(int x, int y, FreeList<Object> &objects)
{
    for (auto &object : iter(objects))
    {
        if (point_in_dimensions(x, y, object_dimensions(object)))
        {
            return &object;
        }
    }

    return nullptr;
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

    for (auto &object : iter(objects))
    {
        world.space.claim_region_rect(object_dimensions(object));
        Object *obj = world.objects.alloc();
        *obj = object;
    }

    return world;
}

Object *Place::place_object(Object object)
{
    RectI region = object_dimensions(object);

    if (space.is_region_claimed(region))
    {
        return nullptr;
    }

    space.claim_region_rect(region);

    Object *obj = objects.alloc();
    *obj = object;

    return obj;
}

bool Place::can_place_building(int floors, int x, int y)
{
    RectI region = {x - 4, y - 4, 8, 8};

    return !space.is_region_claimed(region);
}

bool Place::can_place_objtype(Object::Type type, int x, int y)
{
    Object mock = {};
    mock.type = type;
    mock.x = x;
    mock.y = y;
    return !space.is_region_claimed(object_bounds(mock));
}

void Place::remove_object(int x, int y)
{
    if (Object *obj = object_space(x, y, objects); obj != nullptr)
    {
        space.unclaim_region_rect(object_dimensions(*obj));
        objects.free(obj);
    }
}

Object *Place::get_object_at(int x, int y)
{
    return object_space(x, y, objects);
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

    for (auto &place : iter(places))
    {
        if (&place == first)
        {
            world.first = world.places.alloc();
            *world.first = place.clone();
            world.current = world.first;
            continue;
        }

        *world.places.alloc() = place.clone();
    }

    return world;
}
