#include "catedu/gui/editor/camera.hpp"
#include "catedu/gui/transition/transition.hpp"
#include "catedu/scene/physics.hpp"
#include "catedu/scene/world.hpp"
#include "catedu/ui/user.hpp"
#include <catedu/sys/input.hpp>

struct Playtest
{
    TableId player;
    PhysicsWorld physics;
    World world;
    Place *switch_target;
    ScriptNode *current;
    ScriptNode *pending;

    static Playtest create(World world);
    void destroy();

    void update(UiPass &user, Input &input, EditorCamera &camera,
                GuiTransition &transition);
};
