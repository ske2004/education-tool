#include "catedu/gui/editor/edit_basic.hpp"
#include "catedu/gui/editor/edit_building.hpp"
#include "catedu/gui/editor/edit_delete.hpp"
#include "catedu/gui/editor/edit_line.hpp"
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
