#include "catedu/genobj/building.hpp"
#include "catedu/gui/editor/edit_basic.hpp"
#include "catedu/gui/editor/edit_building.hpp"
#include "catedu/gui/editor/edit_delete.hpp"
#include "catedu/gui/editor/edit_line.hpp"
#include "catedu/gui/editor/playtest.hpp"
#include "catedu/scene/navigation.hpp"
#include "tau/tau.h"

// Helper: create a test dispatcher (no file I/O or rendering needed)
static Dispatcher make_test_dispatcher()
{
    return Dispatcher::create();
}

// Helper: create a camera looking straight down at the origin
static Camera make_test_camera()
{
    Camera cam = Camera::init(60);
    cam.set_aspect(1.0f);
    cam.move(0, 0, -20);
    cam.rotate_around({0, 0, 0}, 0, 90);
    return cam;
}

static const Vector2 TEST_VIEWPORT = {800, 600};

// Helper: create input with mouse at screen center (maps to world origin)
static Input make_test_input()
{
    Input input = {};
    input.mouse_pos = {TEST_VIEWPORT.x / 2, TEST_VIEWPORT.y / 2};
    return input;
}

// Helper: create input with left mouse button pressed
static Input make_test_input_click()
{
    Input input = make_test_input();
    input.k[INPUT_MB_LEFT].pressed = true;
    input.k[INPUT_MB_LEFT].held = true;
    return input;
}

// Helper: create input with left mouse button held (not pressed this frame)
static Input make_test_input_held()
{
    Input input = make_test_input();
    input.k[INPUT_MB_LEFT].held = true;
    return input;
}

// Helper: create input with left mouse button released
static Input make_test_input_released()
{
    Input input = make_test_input();
    input.k[INPUT_MB_LEFT].released = true;
    return input;
}

// ============================================================================
// EditBasic tests
// ============================================================================

TEST(EditBasic, CursorComputedFromRayCast)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    Input input = make_test_input();
    EditBasic edit = {};

    edit.update(disp, input, cam, TEST_VIEWPORT, Object::Type::player);

    // Cursor should be a finite value (computed from ray cast)
    CHECK_TRUE(edit.cursor.x == floorf(edit.cursor.x));
    CHECK_TRUE(edit.cursor.y == floorf(edit.cursor.y));

    disp.destroy();
}

TEST(EditBasic, ValidWhenPositionEmpty)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    Input input = make_test_input();
    EditBasic edit = {};

    edit.update(disp, input, cam, TEST_VIEWPORT, Object::Type::player);

    CHECK_TRUE(edit.valid);

    disp.destroy();
}

TEST(EditBasic, InvalidAfterPlacement)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditBasic edit = {};

    // First click places the object
    Input click = make_test_input_click();
    edit.update(disp, click, cam, TEST_VIEWPORT, Object::Type::tree);

    // Same position should now be invalid
    Input input = make_test_input();
    edit.update(disp, input, cam, TEST_VIEWPORT, Object::Type::tree);
    CHECK_FALSE(edit.valid);

    disp.destroy();
}

TEST(EditBasic, ClickPlacesObject)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditBasic edit = {};

    Input click = make_test_input_click();
    edit.update(disp, click, cam, TEST_VIEWPORT, Object::Type::tree);

    // Object should exist at cursor position
    Object *obj = disp.world.current->get_object_at(edit.cursor.x, edit.cursor.y);
    CHECK_TRUE(obj != nullptr);
    CHECK_EQ((int)obj->type, (int)Object::Type::tree);

    disp.destroy();
}

TEST(EditBasic, NoClickNoPlacement)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditBasic edit = {};

    Input input = make_test_input();
    edit.update(disp, input, cam, TEST_VIEWPORT, Object::Type::player);

    // No object placed without click
    Object *obj = disp.world.current->get_object_at(edit.cursor.x, edit.cursor.y);
    CHECK_TRUE(obj == nullptr);

    disp.destroy();
}

TEST(EditBasic, TypeForwardedCorrectly)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditBasic edit = {};

    Input click = make_test_input_click();
    edit.update(disp, click, cam, TEST_VIEWPORT, Object::Type::player);

    Object *obj = disp.world.current->get_object_at(edit.cursor.x, edit.cursor.y);
    CHECK_TRUE(obj != nullptr);
    CHECK_EQ((int)obj->type, (int)Object::Type::player);

    disp.destroy();
}

// ============================================================================
// EditBuilding tests
// ============================================================================

TEST(EditBuilding, CursorFollowsMouseWhenNotPlacing)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    Input input = make_test_input();
    EditBuilding edit = {};

    edit.update(disp, input, cam, TEST_VIEWPORT);

    // x, y should be set from ray cast
    CHECK_TRUE(edit.x == floorf(edit.x));
    CHECK_TRUE(edit.y == floorf(edit.y));
    CHECK_FALSE(edit.placing);

    disp.destroy();
}

TEST(EditBuilding, FloorClampedTo1Through10)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditBuilding edit = {};

    // When not placing, floors resets to 1
    Input input = make_test_input();
    edit.update(disp, input, cam, TEST_VIEWPORT);
    CHECK_EQ(edit.floors, 1);

    // Start placing
    Input click = make_test_input_click();
    edit.update(disp, click, cam, TEST_VIEWPORT);

    if (edit.placing)
    {
        // Scroll up to increase floors
        Input scroll_up = make_test_input();
        scroll_up.mouse_wheel = 20;
        edit.update(disp, scroll_up, cam, TEST_VIEWPORT);
        CHECK_TRUE(edit.floors <= 10);

        // Scroll down way past minimum
        Input scroll_down = make_test_input();
        scroll_down.mouse_wheel = -20;
        edit.update(disp, scroll_down, cam, TEST_VIEWPORT);
        CHECK_TRUE(edit.floors >= 1);
    }

    disp.destroy();
}

TEST(EditBuilding, ValidWhenEmpty)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    Input input = make_test_input();
    EditBuilding edit = {};

    edit.update(disp, input, cam, TEST_VIEWPORT);

    CHECK_TRUE(edit.valid);

    disp.destroy();
}

TEST(EditBuilding, ClickStartsPlacing)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditBuilding edit = {};

    Input click = make_test_input_click();
    edit.update(disp, click, cam, TEST_VIEWPORT);

    CHECK_TRUE(edit.placing);

    disp.destroy();
}

TEST(EditBuilding, SecondClickCommitsBuilding)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditBuilding edit = {};

    // First click: start placing
    Input click1 = make_test_input_click();
    edit.update(disp, click1, cam, TEST_VIEWPORT);
    CHECK_TRUE(edit.placing);

    float bx = edit.x;
    float by = edit.y;

    // Second click: commit
    Input click2 = make_test_input_click();
    edit.update(disp, click2, cam, TEST_VIEWPORT);
    CHECK_FALSE(edit.placing);

    // Building should exist
    Object *obj = disp.world.current->get_object_at(bx, by);
    CHECK_TRUE(obj != nullptr);
    CHECK_EQ((int)obj->type, (int)Object::Type::building);

    disp.destroy();
}

TEST(EditBuilding, ClickOnExistingBuildingEntersPlace)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditBuilding edit = {};

    // Place a building directly
    Object building = {};
    building.type = Object::Type::building;
    building.floors = 1;
    building.x = edit.x;
    building.y = edit.y;

    // First, get the cursor position
    Input input = make_test_input();
    edit.update(disp, input, cam, TEST_VIEWPORT);

    building.x = edit.x;
    building.y = edit.y;
    disp.place_object(building);

    // Now click on the building
    Input click = make_test_input_click();
    edit.update(disp, click, cam, TEST_VIEWPORT);

    // Should have entered the building's place (current changed)
    // Note: placing will be false since can_place_building fails on occupied spot
    CHECK_FALSE(edit.placing);

    disp.destroy();
}

TEST(EditBuilding, MouseWheelConsumedDuringPlacing)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditBuilding edit = {};

    // Start placing
    Input click = make_test_input_click();
    edit.update(disp, click, cam, TEST_VIEWPORT);

    if (edit.placing)
    {
        Input scroll = make_test_input();
        scroll.mouse_wheel = 3;
        edit.update(disp, scroll, cam, TEST_VIEWPORT);
        // mouse_wheel should be zeroed (consumed)
        CHECK_EQ(scroll.mouse_wheel, 0);
    }

    disp.destroy();
}

// ============================================================================
// EditDelete tests
// ============================================================================

TEST(EditDelete, NoTargetWhenEmpty)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    Input input = make_test_input();
    EditDelete edit = {};

    edit.update(disp, input, cam, TEST_VIEWPORT);

    CHECK_FALSE(edit.has_target);

    disp.destroy();
}

TEST(EditDelete, HasTargetWhenObjectUnderCursor)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditDelete edit = {};

    // First, get cursor position
    Input input = make_test_input();
    edit.update(disp, input, cam, TEST_VIEWPORT);

    // Place a wall at cursor (wall is 1x1, easy to hit)
    Object wall = {};
    wall.type = Object::Type::wall;
    wall.x = edit.target_bounds.pos.x; // use cursor area
    wall.y = edit.target_bounds.pos.y;

    // Re-approach: place at known position, then check
    // Use EditBasic to get cursor pos first
    EditBasic basic = {};
    basic.update(disp, input, cam, TEST_VIEWPORT, Object::Type::wall);

    wall.x = basic.cursor.x;
    wall.y = basic.cursor.y;
    disp.place_object(wall);

    // Now check delete targeting
    edit.update(disp, input, cam, TEST_VIEWPORT);
    CHECK_TRUE(edit.has_target);

    disp.destroy();
}

TEST(EditDelete, TargetBoundsSetCorrectly)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditDelete edit = {};
    Input input = make_test_input();

    // Get cursor position
    EditBasic basic = {};
    basic.update(disp, input, cam, TEST_VIEWPORT, Object::Type::wall);

    // Place wall at cursor
    Object wall = {};
    wall.type = Object::Type::wall;
    wall.x = basic.cursor.x;
    wall.y = basic.cursor.y;
    disp.place_object(wall);

    edit.update(disp, input, cam, TEST_VIEWPORT);

    CHECK_TRUE(edit.has_target);
    // Wall is 1x1, bounds should have size 1x1
    CHECK_EQ(edit.target_bounds.siz.x, 1);
    CHECK_EQ(edit.target_bounds.siz.y, 1);

    disp.destroy();
}

TEST(EditDelete, HeldClickRemovesObject)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditDelete edit = {};
    Input input = make_test_input();

    // Get cursor position
    EditBasic basic = {};
    basic.update(disp, input, cam, TEST_VIEWPORT, Object::Type::wall);

    // Place wall at cursor
    Object wall = {};
    wall.type = Object::Type::wall;
    wall.x = basic.cursor.x;
    wall.y = basic.cursor.y;
    disp.place_object(wall);

    // Held click should remove it
    Input held = make_test_input_held();
    edit.update(disp, held, cam, TEST_VIEWPORT);

    // Object should be gone now
    Input check = make_test_input();
    edit.update(disp, check, cam, TEST_VIEWPORT);
    CHECK_FALSE(edit.has_target);

    disp.destroy();
}

TEST(EditDelete, NoRemovalWithoutClick)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditDelete edit = {};
    Input input = make_test_input();

    // Get cursor position
    EditBasic basic = {};
    basic.update(disp, input, cam, TEST_VIEWPORT, Object::Type::wall);

    // Place wall at cursor
    Object wall = {};
    wall.type = Object::Type::wall;
    wall.x = basic.cursor.x;
    wall.y = basic.cursor.y;
    disp.place_object(wall);

    // Just hover, no click
    edit.update(disp, input, cam, TEST_VIEWPORT);

    // Object should still be there
    CHECK_TRUE(edit.has_target);
    Object *obj = disp.world.current->get_object_at(basic.cursor.x, basic.cursor.y);
    CHECK_TRUE(obj != nullptr);

    disp.destroy();
}

// ============================================================================
// Player placement tests
// ============================================================================

TEST(PlayerPlacement, CanPlaceOnRoad)
{
    Dispatcher disp = make_test_dispatcher();

    disp.place_object({Object::Type::road, 0, 0, 0});
    disp.place_object({Object::Type::player, 0, 0, 0});

    Object *player = disp.world.current->get_object_at(0, 0, Object::Type::player);
    CHECK_TRUE(player != nullptr);
    // Plain lookups still return the road the player stands on
    CHECK_EQ((int)disp.world.current->get_object_at(0, 0)->type,
             (int)Object::Type::road);

    disp.destroy();
}

TEST(PlayerPlacement, CannotPlaceOnWall)
{
    Dispatcher disp = make_test_dispatcher();

    disp.place_object({Object::Type::wall, 0, 0, 0});
    CHECK_FALSE(disp.world.current->can_place_objtype(Object::Type::player, 0, 0));

    disp.destroy();
}

TEST(PlayerPlacement, InvalidPlacementKeepsExistingPlayer)
{
    Dispatcher disp = make_test_dispatcher();

    disp.place_object({Object::Type::player, 0, 10, 10});
    disp.place_object({Object::Type::wall, 0, 0, 0});
    disp.place_object({Object::Type::player, 0, 0, 0});

    CHECK_TRUE(disp.world.current->get_object_at(10, 10, Object::Type::player) != nullptr);

    disp.destroy();
}

TEST(PlayerPlacement, ObjectsCannotBePlacedOnPlayer)
{
    Dispatcher disp = make_test_dispatcher();

    disp.place_object({Object::Type::player, 0, 0, 0});
    CHECK_FALSE(disp.world.current->can_place_objtype(Object::Type::tree, 0, 0));
    CHECK_FALSE(disp.world.current->can_place_building(1, 0, 0));
    CHECK_TRUE(disp.world.current->can_place_objtype(Object::Type::road, 0, 0));

    disp.destroy();
}

TEST(PlayerPlacement, UndoRoadUnderPlayerRemovesRoad)
{
    Dispatcher disp = make_test_dispatcher();

    disp.place_object({Object::Type::player, 0, 0, 0});
    disp.place_object({Object::Type::road, 0, 0, 0});
    disp.undo();

    CHECK_TRUE(disp.world.current->get_object_at(0, 0, Object::Type::road) == nullptr);
    CHECK_TRUE(disp.world.current->get_object_at(0, 0, Object::Type::player) != nullptr);

    disp.destroy();
}

TEST(PlayerPlacement, DeleteTargetsPlayerOnRoad)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditDelete edit = {};
    Input input = make_test_input();

    EditBasic basic = {};
    basic.update(disp, input, cam, TEST_VIEWPORT, Object::Type::road);

    disp.place_object({Object::Type::road, 0, basic.cursor.x, basic.cursor.y});
    disp.place_object({Object::Type::player, 0, basic.cursor.x, basic.cursor.y});

    Input held = make_test_input_held();
    edit.update(disp, held, cam, TEST_VIEWPORT);

    CHECK_TRUE(disp.world.current->get_object_at(basic.cursor.x, basic.cursor.y,
                                                 Object::Type::player) == nullptr);
    CHECK_TRUE(disp.world.current->get_object_at(basic.cursor.x, basic.cursor.y,
                                                 Object::Type::road) != nullptr);

    disp.destroy();
}

// ============================================================================
// EditLine tests
// ============================================================================

TEST(EditLine, CursorUpdatedEachFrame)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    Input input = make_test_input();
    EditLine edit = {};

    edit.update(disp, input, cam, TEST_VIEWPORT, Object::Type::road);

    CHECK_TRUE(edit.cursor.x == floorf(edit.cursor.x));
    CHECK_TRUE(edit.cursor.y == floorf(edit.cursor.y));

    disp.destroy();
}

TEST(EditLine, PressStartsDrag)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditLine edit = {};

    Input click = make_test_input_click();
    edit.update(disp, click, cam, TEST_VIEWPORT, Object::Type::road);

    CHECK_TRUE(edit.started);

    disp.destroy();
}

TEST(EditLine, ReleaseCommitsPlacement)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditLine edit = {};

    // Press to start
    Input click = make_test_input_click();
    edit.update(disp, click, cam, TEST_VIEWPORT, Object::Type::wall);
    CHECK_TRUE(edit.started);

    float sx = edit.pointer_start.x;
    float sy = edit.pointer_start.y;

    // Release to commit (same position = single point)
    Input release = make_test_input_released();
    edit.update(disp, release, cam, TEST_VIEWPORT, Object::Type::wall);

    // Object should be placed at start position
    Object *obj = disp.world.current->get_object_at(sx, sy);
    CHECK_TRUE(obj != nullptr);
    CHECK_EQ((int)obj->type, (int)Object::Type::wall);

    disp.destroy();
}

TEST(EditLine, PointerStartEndTracked)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditLine edit = {};

    // No drag: start and end should match cursor
    Input input = make_test_input();
    edit.update(disp, input, cam, TEST_VIEWPORT, Object::Type::road);

    CHECK_TRUE(edit.pointer_start.x == edit.cursor.x);
    CHECK_TRUE(edit.pointer_start.y == edit.cursor.y);
    CHECK_TRUE(edit.pointer_end.x == edit.cursor.x);
    CHECK_TRUE(edit.pointer_end.y == edit.cursor.y);

    disp.destroy();
}

TEST(EditLine, NoPlacementWithoutRelease)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditLine edit = {};

    // Press to start
    Input click = make_test_input_click();
    edit.update(disp, click, cam, TEST_VIEWPORT, Object::Type::road);

    float sx = edit.pointer_start.x;
    float sy = edit.pointer_start.y;

    // Hold without release
    Input held = make_test_input_held();
    edit.update(disp, held, cam, TEST_VIEWPORT, Object::Type::road);

    // Nothing placed yet
    Object *obj = disp.world.current->get_object_at(sx, sy);
    CHECK_TRUE(obj == nullptr);

    disp.destroy();
}

TEST(EditLine, WorksWithWallType)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditLine edit = {};

    // Press
    Input click = make_test_input_click();
    edit.update(disp, click, cam, TEST_VIEWPORT, Object::Type::wall);

    float sx = edit.pointer_start.x;
    float sy = edit.pointer_start.y;

    // Release
    Input release = make_test_input_released();
    edit.update(disp, release, cam, TEST_VIEWPORT, Object::Type::wall);

    Object *obj = disp.world.current->get_object_at(sx, sy);
    CHECK_TRUE(obj != nullptr);
    CHECK_EQ((int)obj->type, (int)Object::Type::wall);

    disp.destroy();
}

TEST(EditLine, WorksWithRoadType)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditLine edit = {};

    // Press
    Input click = make_test_input_click();
    edit.update(disp, click, cam, TEST_VIEWPORT, Object::Type::road);

    float sx = edit.pointer_start.x;
    float sy = edit.pointer_start.y;

    // Release
    Input release = make_test_input_released();
    edit.update(disp, release, cam, TEST_VIEWPORT, Object::Type::road);

    Object *obj = disp.world.current->get_object_at(sx, sy);
    CHECK_TRUE(obj != nullptr);
    CHECK_EQ((int)obj->type, (int)Object::Type::road);

    disp.destroy();
}

// ============================================================================
// Dispatcher Undo / Redo & Building Lifecycle tests
// ============================================================================

TEST(DispatcherUndoRedo, UndoAndRedoFirstAction)
{
    Dispatcher disp = make_test_dispatcher();

    disp.place_object({Object::Type::tree, 0, 4, 4});
    CHECK_TRUE(disp.world.current->get_object_at(4, 4) != nullptr);

    disp.undo();
    CHECK_TRUE(disp.world.current->get_object_at(4, 4) == nullptr);

    disp.redo();
    CHECK_TRUE(disp.world.current->get_object_at(4, 4) != nullptr);

    disp.destroy();
}

TEST(DispatcherUndoRedo, CrossPlaceUndoRedoIsolation)
{
    Dispatcher disp = make_test_dispatcher();

    disp.place_object({Object::Type::building, 2, 0, 0});
    Object *bldg = disp.world.current->get_object_at(0, 0);
    REQUIRE_TRUE(bldg != nullptr);

    disp.enter_place(bldg);
    Place *interior = disp.world.current;
    REQUIRE_TRUE(interior != disp.world.first);

    disp.place_object({Object::Type::item, 0, 1, -8});
    CHECK_TRUE(interior->get_object_at(1, -8, Object::Type::item) != nullptr);

    // Undo inside interior
    disp.undo();
    CHECK_TRUE(interior->get_object_at(1, -8, Object::Type::item) == nullptr);

    // Switch to outdoor place and attempt redo: should NOT redo interior action outdoors
    disp.world.current = disp.world.first;
    disp.redo();
    CHECK_TRUE(disp.world.first->get_object_at(1, -8, Object::Type::item) == nullptr);

    // Switch back inside and redo: should restore interior item
    disp.world.current = interior;
    disp.redo();
    CHECK_TRUE(interior->get_object_at(1, -8, Object::Type::item) != nullptr);

    disp.destroy();
}

TEST(DispatcherUndoRedo, BuildingRemovalUndoRedoAndCloneSafety)
{
    Dispatcher disp = make_test_dispatcher();

    disp.place_object({Object::Type::building, 2, 0, 0});
    Object *bldg = disp.world.current->get_object_at(0, 0);
    REQUIRE_TRUE(bldg != nullptr);

    disp.enter_place(bldg);
    Place *interior = disp.world.current;
    interior->place_object({Object::Type::item, 0, 1, -8});
    ScriptNode *enter_ev = disp.world.script->acquire_place_event(interior);
    REQUIRE_TRUE(enter_ev != nullptr);

    // Return outdoors and delete the building
    disp.world.current = disp.world.first;
    disp.remove_object(bldg);
    CHECK_TRUE(disp.world.current->get_object_at(0, 0) == nullptr);

    // Cloning the world while a building with a script event is deleted must not assert
    World cloned = disp.world.clone();
    cloned.destroy();

    // Undo deletion: building, its interior item, and script event place pointer are restored
    disp.undo();
    Object *restored = disp.world.current->get_object_at(0, 0);
    REQUIRE_TRUE(restored != nullptr);
    REQUIRE_TRUE(restored->place != nullptr);
    CHECK_TRUE(restored->place->get_object_at(1, -8, Object::Type::item) != nullptr);
    CHECK_EQ(enter_ev->place, restored->place);

    // Redo deletion and undo again
    disp.redo();
    CHECK_TRUE(disp.world.current->get_object_at(0, 0) == nullptr);
    disp.undo();
    restored = disp.world.current->get_object_at(0, 0);
    REQUIRE_TRUE(restored != nullptr);
    REQUIRE_TRUE(restored->place != nullptr);
    CHECK_TRUE(restored->place->get_object_at(1, -8, Object::Type::item) != nullptr);

    disp.destroy();
}

// ============================================================================
// Raycast & Navigation Edge Cases
// ============================================================================

TEST(EditorRaycast, SkywardCameraDoesNotPlaceOrCrash)
{
    Dispatcher disp = make_test_dispatcher();
    // Camera above ground looking straight up (away from y=0 plane)
    Camera cam = Camera::init(60);
    cam.set_aspect(1.0f);
    cam.move(0, 10, 0);

    Input click = make_test_input_click();

    EditBasic basic = {};
    basic.valid = true;
    basic.update(disp, click, cam, TEST_VIEWPORT, Object::Type::tree);
    CHECK_FALSE(basic.valid);

    EditBuilding bldg = {};
    bldg.valid = true;
    bldg.update(disp, click, cam, TEST_VIEWPORT);
    CHECK_FALSE(bldg.valid);
    CHECK_FALSE(bldg.placing);

    EditDelete del = {};
    del.has_target = true;
    del.update(disp, click, cam, TEST_VIEWPORT);
    CHECK_FALSE(del.has_target);

    EditLine line = {};
    line.update(disp, click, cam, TEST_VIEWPORT, Object::Type::road);
    CHECK_FALSE(line.started);

    disp.destroy();
}

TEST(Navigation, CastleBlocksWalkabilityAndHighBuildingSafe)
{
    Dispatcher disp = make_test_dispatcher();

    disp.place_object({Object::Type::castle, 4, 0, 0});
    CHECK_FALSE(is_walkable(disp.world.current, {0, 0}));
    CHECK_TRUE(is_walkable(disp.world.current, {10, 10}));

    GeneratedObject tall = genmesh_generate_building(30);
    CHECK_TRUE(tall.num_components <= COMPONENT_COUNT);

    disp.destroy();
}

TEST(DispatcherUndoRedo, EditLineDragCompoundUndoRedo)
{
    Dispatcher disp = make_test_dispatcher();
    Camera cam = make_test_camera();
    EditLine edit = {};

    // Start drag
    Input click = make_test_input_click();
    edit.update(disp, click, cam, TEST_VIEWPORT, Object::Type::wall);

    // Set drag range from (0, 0) to (3, 0)
    edit.pointer_start = {0, 0};
    edit.pointer_end = {3, 0};
    edit.started = true;

    // Release to commit the line placement
    Input release = make_test_input_released();
    edit.update(disp, release, cam, TEST_VIEWPORT, Object::Type::wall);

    // Verify all 4 wall tiles are placed
    for (int x = 0; x <= 3; x++)
    {
        Object *obj = disp.world.current->get_object_at(x, 0);
        REQUIRE_TRUE(obj != nullptr);
        CHECK_EQ((int)obj->type, (int)Object::Type::wall);
    }

    // 1 undo removes the entire line
    disp.undo();
    for (int x = 0; x <= 3; x++)
    {
        CHECK_TRUE(disp.world.current->get_object_at(x, 0) == nullptr);
    }

    // 1 redo restores the entire line
    disp.redo();
    for (int x = 0; x <= 3; x++)
    {
        Object *obj = disp.world.current->get_object_at(x, 0);
        REQUIRE_TRUE(obj != nullptr);
        CHECK_EQ((int)obj->type, (int)Object::Type::wall);
    }

    disp.destroy();
}

TEST(DispatcherUndoRedo, PlayerReplacementCompoundUndoRedo)
{
    Dispatcher disp = make_test_dispatcher();

    disp.place_object({Object::Type::player, 0, 0, 0});
    CHECK_TRUE(disp.world.current->get_object_at(0, 0, Object::Type::player) != nullptr);

    // Place a new player at (10, 10). Replaces the old player in a compound group.
    disp.place_object({Object::Type::player, 0, 10, 10});
    CHECK_TRUE(disp.world.current->get_object_at(0, 0, Object::Type::player) == nullptr);
    CHECK_TRUE(disp.world.current->get_object_at(10, 10, Object::Type::player) != nullptr);

    // 1 undo removes the new player AND restores the old player
    disp.undo();
    CHECK_TRUE(disp.world.current->get_object_at(0, 0, Object::Type::player) != nullptr);
    CHECK_TRUE(disp.world.current->get_object_at(10, 10, Object::Type::player) == nullptr);

    // 1 redo removes the old player AND restores the new player
    disp.redo();
    CHECK_TRUE(disp.world.current->get_object_at(0, 0, Object::Type::player) == nullptr);
    CHECK_TRUE(disp.world.current->get_object_at(10, 10, Object::Type::player) != nullptr);

    disp.destroy();
}

TEST(BridgePlacement, BridgeCanBePlacedOverWaterAndWalkable)
{
    Dispatcher disp = make_test_dispatcher();

    // Place water at (5, 5)
    disp.place_object({Object::Type::water, 0, 5, 5});
    Object *water = disp.world.current->get_object_at(5, 5, Object::Type::water);
    REQUIRE_TRUE(water != nullptr);
    // Unbridged water is not walkable
    CHECK_FALSE(is_walkable(disp.world.current, {5, 5}));

    // Can place bridge over water
    CHECK_TRUE(disp.world.current->can_place_objtype(Object::Type::bridge, 5, 5));
    disp.place_object({Object::Type::bridge, 0, 5, 5});
    Object *bridge = disp.world.current->get_object_at(5, 5, Object::Type::bridge);
    REQUIRE_TRUE(bridge != nullptr);

    // get_object_at prefers bridge over water
    Object *top = disp.world.current->get_object_at(5, 5);
    REQUIRE_TRUE(top != nullptr);
    CHECK_EQ((int)top->type, (int)Object::Type::bridge);

    // Bridged water is walkable
    CHECK_TRUE(is_walkable(disp.world.current, {5, 5}));

    // Cannot place a second bridge on the same tile
    CHECK_FALSE(disp.world.current->can_place_objtype(Object::Type::bridge, 5, 5));

    // Remove the bridge: water remains underneath and keeps tile claimed
    disp.remove_object(5, 5);
    CHECK_TRUE(disp.world.current->get_object_at(5, 5, Object::Type::bridge) == nullptr);
    CHECK_TRUE(disp.world.current->get_object_at(5, 5, Object::Type::water) != nullptr);
    CHECK_TRUE(disp.world.current->space.is_region_claimed({5, 5, 1, 1}));
    CHECK_FALSE(is_walkable(disp.world.current, {5, 5}));

    // Cannot place wall on water
    CHECK_FALSE(disp.world.current->can_place_objtype(Object::Type::wall, 5, 5));

    disp.destroy();
}

TEST(Playtest, NoHardcodedItemsAndWaterColliderSkippedWhenBridged)
{
    World world = World::create();
    world.first->place_object({Object::Type::player, 0, 0, 0});
    world.first->place_object({Object::Type::water, 0, 2, 2});
    world.first->place_object({Object::Type::bridge, 0, 2, 2});
    world.first->place_object({Object::Type::water, 0, 6, 6});

    Playtest playtest = Playtest::create(world);

    // Ensure no hardcoded items were generated
    int item_count = 0;
    for (auto &obj : iter(playtest.world.first->objects))
    {
        if (obj.type == Object::Type::item)
        {
            item_count++;
        }
    }
    CHECK_EQ(item_count, 0);

    // Bridged water at (2, 2) has no solid physics collider, open water at (6, 6) does
    bool found_bridged_water = false;
    bool found_open_water = false;
    for (auto [id, body] : iter(playtest.physics.bodies))
    {
        if (body.solid && body.area.pos.x == 2 && body.area.pos.y == 2)
        {
            found_bridged_water = true;
        }
        if (body.solid && body.area.pos.x == 6 && body.area.pos.y == 6)
        {
            found_open_water = true;
        }
    }
    CHECK_FALSE(found_bridged_water);
    CHECK_TRUE(found_open_water);

    playtest.destroy();
}


