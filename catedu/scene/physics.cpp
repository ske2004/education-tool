#include "physics.hpp"
#include "catedu/core/math/math.hpp"
#include "catedu/core/storage/stack.hpp"

struct IdPair
{
    TableId id;
    PhysicsBody body;
};

static void sort_bodies_to_closest(Stack<IdPair> &bodies, Vector2 pos)
{
    // TODO: Use quicksort instead
    for (size_t i = 0; i < bodies.count; i++)
    {
        float min_distance = MATH_INFINITY;
        size_t candidate = i;

        for (size_t j = i; j < bodies.count; j++)
        {
            float distance =
                vector2_cmp_distance(rect_center(bodies[j].body.area), pos);

            if (distance < min_distance)
            {
                min_distance = distance;
                candidate = j;
            }
        }

        std::swap(bodies[i], bodies[candidate]);
    }
}

static Stack<IdPair> get_bodies_except(PhysicsWorld &world, TableId xid)
{
    Stack<IdPair> r = {};

    for (auto [id, body] : iter(world.bodies))
    {
        if (id != xid)
        {
            r.push({id, body});
        }
    }

    return r;
}

void resolve_bodies(PhysicsBody &a, PhysicsBody &b)
{
    if (a.dynamic && !b.dynamic)
    {
        a.area.pos += rect_vs_rect_snap(a.area, b.area);
    }
    else if (!a.dynamic && b.dynamic)
    {
        b.area.pos += rect_vs_rect_snap(a.area, b.area);
    }
    else if (a.dynamic && b.dynamic)
    {
        Vector2 snap = rect_vs_rect_snap(a.area, b.area);
        a.area.pos += snap * 0.5f;
        b.area.pos -= snap * 0.5f;
    }
}

void PhysicsWorld::resolve_physics(PhysicsManifolds &manifolds)
{
    for (auto [_, manifold] : iter(manifolds.manifolds))
    {
        PhysicsBody &a = bodies.get_assert(manifold.first);
        PhysicsBody &b = bodies.get_assert(manifold.second);

        if (a.solid && b.solid)
        {
            resolve_bodies(a, b);
        }
    }
}

PhysicsManifolds PhysicsWorld::detect_collisions()
{
    PhysicsManifolds r = {};

    for (auto [id, body] : iter(bodies))
    {
        if (body.dynamic)
        {
            Stack<IdPair> bodies = get_bodies_except(*this, id);
            sort_bodies_to_closest(bodies, rect_center(body.area));

            for (auto body2 : iter(bodies))
            {
                if (rect_vs_rect(body.area, body2.body.area))
                {
                    r.manifolds.allocate({id, body2.id});
                }
            }

            bodies.deinit();
        }
    }

    return r;
}
