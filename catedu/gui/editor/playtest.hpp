#include "catedu/gui/editor/camera.hpp"
#include "catedu/gui/transition/transition.hpp"
#include "catedu/scene/physics.hpp"
#include "catedu/scene/world.hpp"
#include "catedu/scene/inventory.hpp"
#include "catedu/ui/user.hpp"
#include <catedu/sys/input.hpp>

enum class NpcState {
    wandering,
    going_to_work,
    working
};

struct PlaytestNpc
{
    TableId body;
    Vector2 movement;
    float timer;
    Object *obj;
    
    Vector2i path[32];
    int path_count;
    int path_index;
    
    NpcState state;
};

struct Playtest
{
    TableId player;
    float player_vz;
    Table<PlaytestNpc> npcs;
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
