extends SceneTree
## Runs the story engine's tests without a window:
##
##   godot --headless --path godot -s res://tests/run_tests.gd
##
## Exits with status 1 if any check fails.

const StoryData = preload("res://scripts/story.gd")
const StoryState = preload("res://scripts/story_state.gd")
const Builder = preload("res://scripts/world_builder.gd")
const Controls = preload("res://scripts/controls.gd")
const PlayMode = preload("res://scripts/play_mode.gd")
const StoryEditor = preload("res://scripts/editor_mode.gd")

const SAMPLE := "res://stories/water_cycle.json"

var checks := 0
var failures := 0
var current := ""


func _initialize() -> void:
	Controls.ensure_actions()
	_run.call_deferred()


func _run() -> void:
	for test in [
		"test_sample_is_ready_to_play",
		"test_blank_story_explains_what_is_missing",
		"test_normalized_fills_defaults",
		"test_save_and_load_round_trip",
		"test_story_links_round_trip",
		"test_parse_reports_bad_files",
		"test_walkthrough_reaches_the_ending",
		"test_conditions_hide_choices",
		"test_problems_catch_broken_links",
		"test_doors_lead_back_to_the_door_you_came_through",
		"test_new_ids_are_unique",
		"test_every_place_builds",
		"test_water_and_weather_build",
	]:
		current = test
		call(test)
	current = "test_play_mode_talk_pick_up_and_travel"
	await test_play_mode_talk_pick_up_and_travel()
	current = "test_play_mode_touch"
	await test_play_mode_touch()
	current = "test_rivers_block_and_bridges_cross"
	await test_rivers_block_and_bridges_cross()
	current = "test_editor_places_moves_and_deletes"
	await test_editor_places_moves_and_deletes()
	current = "test_editor_undo_and_redo"
	await test_editor_undo_and_redo()
	print("%d checks, %d failed" % [checks, failures])
	quit(1 if failures > 0 else 0)


func check(condition: bool, what: String) -> void:
	checks += 1
	if not condition:
		failures += 1
		printerr("FAIL %s: %s" % [current, what])


func sample() -> Dictionary:
	return StoryData.load_file(SAMPLE)["story"]


func test_sample_is_ready_to_play() -> void:
	var result := StoryData.load_file(SAMPLE)
	check(not result.has("error"), "sample loads: %s" % result.get("error", ""))
	check(StoryData.problems(result["story"]).is_empty(), "sample has no problems: %s" % [StoryData.problems(result["story"])])


func test_blank_story_explains_what_is_missing() -> void:
	var problems := StoryData.problems(StoryData.blank())
	check(problems.has("No choice ends the story yet."), "blank story says it has no ending")


func test_normalized_fills_defaults() -> void:
	var story := StoryData.normalized({"places": [{"id": "a", "props": [{"kind": "spaceship"}]}],
			"characters": [{"id": "x", "lines": [{"text": "hi", "choices": [{"text": "bye", "next": 3.0}]}, {}]}]})
	check(story["start_place"] == "a", "start place defaults to the first place")
	check(story["places"][0]["props"][0]["kind"] == "crate", "unknown prop kinds become crates")
	var lines: Array = story["characters"][0]["lines"]
	check(lines[0]["opens"] and not lines[1]["opens"], "only the first line opens by default")
	check(typeof(lines[0]["choices"][0]["next"]) == TYPE_INT, "next becomes an int")
	check(lines[0]["choices"][0]["gives"] == "", "missing choice fields become empty")


func test_save_and_load_round_trip() -> void:
	var story := sample()
	var file := "user://round_trip_test.json"
	check(StoryData.save_file(story, file) == "", "saves")
	var loaded: Dictionary = StoryData.load_file(file)["story"]
	check(JSON.stringify(loaded) == JSON.stringify(story), "loads back the same story")
	DirAccess.remove_absolute(ProjectSettings.globalize_path(file))
	check(StoryData.load_file("user://missing.json").has("error"), "missing files report an error")


func test_story_links_round_trip() -> void:
	var story := sample()
	var code := StoryData.to_code(story)
	check(code.length() < JSON.stringify(story).length() / 2, "a link code is much shorter than the story (%d chars)" % code.length())
	var url_safe := true
	for ch in code:
		if not (ch.is_valid_identifier() or ch.is_valid_int() or ch == "-"):
			url_safe = false
	check(url_safe, "a link code only uses URL-safe characters")
	var result := StoryData.from_code(code)
	check(result.has("story") and JSON.stringify(result["story"]) == JSON.stringify(story), "a link code gives back the same story")
	check(StoryData.from_code(code.substr(0, code.length() / 2)).has("error"), "a cut-off link reports an error")
	check(StoryData.from_code("not a story!").has("error"), "a broken link reports an error")
	check(StoryData.from_code("").has("error"), "an empty link reports an error")


func test_parse_reports_bad_files() -> void:
	check(StoryData.parse("{\"title\": \"x\"}").has("story"), "parses a minimal story")
	check(StoryData.parse("{oops", "a.json")["error"].begins_with("a.json isn't valid JSON"), "names the file with bad JSON")
	check(StoryData.parse("[1, 2]", "b.json").has("error"), "a JSON list isn't a story")


func _talk(state, story: Dictionary, who: String, picks: Array) -> int:
	var character := StoryData.find(story, "characters", who)
	var line: int = state.opening_line(character)
	for pick in picks:
		if line < 0:
			return line
		var options: Array = state.choices(character, line)
		line = state.choose(character, line, options[pick]["index"])
	return line


func test_walkthrough_reaches_the_ending() -> void:
	var story := sample()
	var state = StoryState.new(story)
	var mayor := StoryData.find(story, "characters", "mayor")
	var omar := StoryData.find(story, "characters", "librarian")
	check(state.opening_line(mayor) == 0, "the mayor starts with the problem")
	check(_talk(state, story, "mayor", [0]) == -1, "agreeing ends the first talk")
	check(state.has("quest"), "agreeing sets the quest fact")
	check(state.opening_line(omar) == 0, "Omar points at the notebook first")
	state.pick_up("notebook")
	check(state.inventory == ["notebook"] and state.collected.has("notebook"), "the notebook is in the backpack")
	check(state.opening_line(omar) == 1, "with the notebook, Omar asks a question")
	check(_talk(state, story, "librarian", [1]) == 3, "a wrong answer gets a hint")
	check(_talk(state, story, "librarian", [0, 0]) == -1, "the right answer finishes the lesson")
	check(state.has("learned_cycle"), "the lesson sets learned_cycle")
	check(state.opening_line(omar) == 4, "afterwards Omar sends the student back")
	check(state.opening_line(mayor) == 2, "the mayor asks for the answer")
	_talk(state, story, "mayor", [0, 0, 0])
	check(state.ending.begins_with("The village built rain barrels"), "explaining the cycle ends the story")


func test_conditions_hide_choices() -> void:
	var story := StoryData.normalized({"places": [{"id": "p"}], "items": [{"id": "key", "name": "Key"}],
			"characters": [{"id": "guard", "place": "p", "lines": [{"text": "Halt!", "choices": [
				{"text": "Open up", "requires": "key", "takes": "key", "sets": "opened"},
				{"text": "Please?", "unless": "key"},
			]}]}]})
	var state = StoryState.new(story)
	var guard := StoryData.find(story, "characters", "guard")
	check(state.choices(guard, 0).map(func(c): return c["text"]) == ["Please?"], "without the key only begging is offered")
	state.pick_up("key")
	check(state.choices(guard, 0).map(func(c): return c["text"]) == ["Open up"], "with the key only opening is offered")
	state.choose(guard, 0, 0)
	check(not state.has("key") and state.has("opened"), "the guard takes the key and the door opens")


func test_problems_catch_broken_links() -> void:
	var story := sample()
	story["places"][0]["doors"][0]["to"] = "nowhere"
	story["characters"][0]["lines"][0]["choices"][0]["next"] = 99
	story["characters"][0]["lines"][0]["choices"][0]["gives"] = "unicorn"
	var problems := "\n".join(StoryData.problems(story))
	check(problems.contains("doesn't lead anywhere"), "broken doors are reported")
	check(problems.contains("line 100, which doesn't exist"), "missing lines are reported")
	check(problems.contains("\"unicorn\" isn't an item"), "unknown items are reported")


func test_doors_lead_back_to_the_door_you_came_through() -> void:
	var story := sample()
	var state = StoryState.new(story)
	state.travel("library")
	check(state.place == "library", "travel changes place")
	check(is_zero_approx(state.pos[0]) and is_equal_approx(state.pos[1], 5.0), "students arrive inside the library door: %s" % [state.pos])
	state.travel("village")
	check(is_equal_approx(state.pos[0], 11.0) and is_equal_approx(state.pos[1], -2.8), "and in front of the library outside: %s" % [state.pos])


func test_new_ids_are_unique() -> void:
	var story := sample()
	check(StoryData.new_id(story, "characters", "Mayor!") == "mayor_2", "taken ids get a number")
	check(StoryData.new_id(story, "items", "Shiny Key") == "shiny_key", "names become ids")
	check(StoryData.slug("  ") == "thing", "empty names still give an id")


func test_every_place_builds() -> void:
	var story := sample()
	for place in story["places"]:
		var node := Builder.build_place(story, place["id"], {}, true)
		var entities: Dictionary = node.get_meta("entities")
		check(entities.keys().filter(func(k): return k.begins_with("prop:")).size() == place["props"].size(), "%s builds every prop" % place["name"])
		node.free()
	var village := Builder.build_place(story, "village")
	var kinds: Array = village.get_meta("interactables").map(func(e): return e["kind"])
	check(kinds.count("character") == 2 and kinds.count("door") == 1, "the village has two people and a door")
	check(kinds.count("seat") == 1, "the village bench can be sat on")
	check(not village.get_meta("entities").has("start"), "the start marker only shows while editing")
	village.free()
	var library := Builder.build_place(story, "library", {"notebook": true})
	check(not library.get_meta("entities").has("item:notebook"), "collected items stay gone")
	library.free()


func test_water_and_weather_build() -> void:
	var story := StoryData.normalized({"places": [{"id": "a", "weather": "hail"}, {"id": "b", "weather": "rain"}]})
	check(story["places"][0]["weather"] == "clear", "unknown weather becomes clear")
	check(story["places"][1]["weather"] == "rain", "rain stays rain")
	for weather in StoryData.WEATHERS:
		story["places"][0]["weather"] = weather
		story["places"][0]["props"] = []
		for kind in ["pond", "river", "bridge", "fountain"]:
			story["places"][0]["props"].append({"kind": kind, "pos": [0, 0], "rot": 0, "color": StoryData.PROP_COLORS[kind]})
		var node := Builder.build_place(story, "a")
		var rain := node.get_children().filter(func(n): return n is CPUParticles3D)
		check(rain.size() == (1 if weather == "rain" else 0), "%s weather has %s rain" % [weather, "" if weather == "rain" else "no"])
		var clouds := node.get_children().filter(func(n): return n.get_script() == Builder.Drifter)
		check(clouds.size() == Builder.WEATHER[weather]["clouds"], "%s weather has %d clouds" % [weather, clouds.size()])
		node.free()
	story["places"][0]["indoor"] = true
	var room := Builder.build_place(story, "a")
	check(room.get_children().filter(func(n): return n.get_script() == Builder.Drifter).is_empty(), "rooms have no clouds")
	room.free()


## Walks right for two seconds from x = -4 across a river running along z at
## x = 0, and returns where the student ends up.
func _walk_across(props: Array) -> Vector3:
	var story := StoryData.normalized({"places": [{"id": "a", "size": 30, "props": props}],
			"start_place": "a", "start_pos": [-4, 0]})
	var play = PlayMode.new()
	play.setup(story)
	root.add_child(play)
	await _frames(3)
	play._close_card()
	Input.action_press("move_right")
	for i in 120:
		await physics_frame
	Input.action_release("move_right")
	var at: Vector3 = play.player.global_position
	play.queue_free()
	await _frames(2)
	return at


func test_rivers_block_and_bridges_cross() -> void:
	var river := {"kind": "river", "pos": [0, 0], "rot": 0}
	var blocked := await _walk_across([river])
	check(blocked.x < -1.5, "a river stops the student (x = %.2f)" % blocked.x)
	var crossed := await _walk_across([river, {"kind": "bridge", "pos": [0, 0], "rot": 0}])
	check(crossed.x > 3.0, "a bridge takes the student across (x = %.2f)" % crossed.x)
	check(absf(crossed.z) < 0.5, "the bridge's rails keep the student on it (z = %.2f)" % crossed.z)


func _frames(n: int) -> void:
	for i in n:
		await process_frame


func test_play_mode_talk_pick_up_and_travel() -> void:
	var play = PlayMode.new()
	play.setup(sample())
	root.add_child(play)
	await _frames(3)
	check(play.overlay.visible, "the intro card shows first")
	play._close_card()
	var mayor: Dictionary = play.place_node.get_meta("interactables").filter(func(e): return e["id"] is String and e["id"] == "mayor")[0]
	play._activate(mayor)
	check(play.dialogue.visible and play.busy, "talking opens the dialogue")
	play.dialogue.pick(0)
	play.dialogue.pick(0)
	await _frames(2)
	check(not play.dialogue.visible and play.state.has("quest"), "picking a choice ends the talk and sets the fact")

	var door: Dictionary = play.place_node.get_meta("interactables").filter(func(e): return e["kind"] == "door")[0]
	play._activate(door)
	await create_timer(0.8).timeout
	check(play.state.place == "library", "the door leads into the library")
	check(not play.busy, "play resumes after the door")
	var notebook: Dictionary = play.place_node.get_meta("interactables").filter(func(e): return e["kind"] == "item")[0]
	play._activate(notebook)
	check(play.state.inventory.has("notebook"), "picking up adds to the backpack")
	check(play.overlay.visible, "the item's description is shown")
	play._close_card()
	await _frames(2)
	play.queue_free()
	await _frames(1)


## Touches go through the root viewport like real ones, so the HUD gets first pick.
func _touch(index: int, at: Vector2, pressed := true) -> void:
	var event := InputEventScreenTouch.new()
	event.index = index
	event.position = at
	event.pressed = pressed
	root.push_input(event, true)


func _drag(index: int, from: Vector2, to: Vector2) -> void:
	var event := InputEventScreenDrag.new()
	event.index = index
	event.position = to
	event.relative = to - from
	root.push_input(event, true)


func _walk_input() -> Vector2:
	return Input.get_vector("move_left", "move_right", "move_forward", "move_back")


func test_play_mode_touch() -> void:
	var play = PlayMode.new()
	play.setup(sample())
	root.add_child(play)
	await _frames(3)
	play._close_card()
	await _frames(1)
	check(not play.stick.visible, "no joystick before the screen is touched")

	var stick_at: Vector2 = play.stick.get_global_rect().get_center()
	_touch(0, stick_at)
	check(play.touch and play.stick.visible, "the first touch shows the joystick")
	var start: Vector3 = play.player.global_position
	var yaw: float = play.rig.yaw
	_drag(0, stick_at, stick_at + Vector2(0, -100))
	check(_walk_input().y < -0.9, "pushing the stick up holds move_forward: %s" % _walk_input())
	await create_timer(0.4).timeout
	var moved: Vector3 = play.player.global_position - start
	var forward := Vector3.FORWARD.rotated(Vector3.UP, play.rig.yaw)
	check(moved.dot(forward) > 0.5, "the stick walks the student forward: %s" % moved)
	check(play.rig.yaw == yaw, "the stick doesn't turn the camera")
	_touch(0, stick_at + Vector2(0, -100), false)
	check(_walk_input() == Vector2.ZERO, "letting go of the stick stops the student")

	var spot: Vector2 = root.get_visible_rect().size * Vector2(0.5, 0.4)
	_touch(0, spot)
	_drag(0, spot, spot + Vector2(120, 0))
	check(play.rig.yaw < yaw - 0.5, "a one-finger drag turns the camera")
	var distance: float = play.rig.distance
	_touch(1, spot + Vector2(200, 0))
	_drag(1, spot + Vector2(200, 0), spot + Vector2(400, 0))
	check(play.rig.distance < distance, "spreading two fingers zooms in")
	_touch(1, spot + Vector2(400, 0), false)
	_touch(0, spot + Vector2(120, 0), false)

	yaw = play.rig.yaw
	var exit_at: Vector2 = play.exit_button.get_global_rect().get_center()
	_touch(0, exit_at)
	_drag(0, exit_at, spot)
	_touch(0, spot, false)
	check(play.rig.yaw == yaw, "a drag that starts on a button doesn't turn the camera")

	check(play.jump_button.visible, "the Jump button shows with the joystick")
	var jump_at: Vector2 = play.jump_button.get_global_rect().get_center()
	_touch(1, jump_at)
	_touch(1, jump_at, false)
	var top: float = await _hold("", 15, play.player)
	check(top > 0.5, "tapping Jump jumps (%.2f)" % top)
	check(play.rig.yaw == yaw, "tapping Jump doesn't turn the camera")
	await _physics(40)

	var mayor: Dictionary = play.place_node.get_meta("interactables").filter(func(e): return e["id"] is String and e["id"] == "mayor")[0]
	play._activate(mayor)
	await _frames(2)
	check(play.dialogue.typing(), "the mayor starts talking")
	check(not play.jump_button.visible, "the Jump button hides while talking")
	var box_at: Vector2 = play.dialogue.get_global_rect().get_center()
	_touch(0, box_at)
	_touch(0, box_at, false)
	check(not play.dialogue.typing() and play.dialogue.options.get_child_count() > 0, "tapping the box finishes the line")
	check(play.rig.yaw == yaw, "tapping the box doesn't turn the camera")
	await _frames(2)
	var choice_at: Vector2 = play.dialogue.options.get_child(0).get_global_rect().get_center()
	_touch(0, choice_at)
	_touch(0, choice_at, false)
	await _frames(1)
	check(not play.dialogue.visible and play.state.has("quest"), "tapping a choice picks it")
	play.queue_free()
	PlayMode.touch = false
	await _frames(1)


func test_editor_places_moves_and_deletes() -> void:
	var editor = StoryEditor.new()
	root.add_child(editor)
	editor.load_story(sample(), SAMPLE)
	await _frames(2)
	var props_before: int = editor._place()["props"].size()
	editor._set_tool("prop:tree")
	editor._click(Vector2(2.2, 3.1), false)
	check(editor._place()["props"].size() == props_before + 1, "the tree tool adds a tree")
	check(editor._place()["props"][-1]["pos"] == [2.0, 3.0], "placing snaps to the half-metre grid")
	check(editor.tool == "select", "the tool goes back to Select")
	check(editor.selected["kind"] == "prop", "the new tree is selected")
	await _frames(2)
	check(editor.inspector.get_child_count() > 0, "the inspector shows the tree")

	editor._click(Vector2(-2.0, -0.5), false)
	check(editor.selected == {"kind": "character", "id": "mayor"}, "clicking the mayor selects her")
	editor._drag_to(Vector2(-4.0, 1.0))
	check(StoryData.find(editor.story, "characters", "mayor")["pos"] == [-4.0, 1.0], "dragging moves her")
	await _frames(2)
	check(editor.inspector.get_child_count() > 10, "the inspector shows her dialogue")

	editor._select("prop", editor._place()["props"].size() - 1)
	editor._delete_selected()
	check(editor._place()["props"].size() == props_before, "Delete removes the tree")
	for kind in ["story", "place"]:
		editor._select(kind, editor.place_id if kind == "place" else "")
		await _frames(1)
		check(editor.inspector.get_child_count() > 0, "the %s inspector builds" % kind)
	editor._go_to("item", "notebook")
	await _frames(1)
	check(editor.place_id == "library", "selecting an item shows its place")
	check(editor.dirty, "edits mark the story unsaved")
	editor.queue_free()
	await _frames(1)


func _ctrl_key(keycode: Key, shift := false) -> InputEventKey:
	var event := InputEventKey.new()
	event.keycode = keycode
	event.ctrl_pressed = true
	event.shift_pressed = shift
	event.pressed = true
	return event


func test_editor_undo_and_redo() -> void:
	var editor = StoryEditor.new()
	root.add_child(editor)
	editor.load_story(sample(), SAMPLE)
	await _frames(2)
	check(editor.undo_button.disabled and editor.redo_button.disabled, "a fresh story has nothing to undo or redo")

	var props_before: int = editor._place()["props"].size()
	editor._set_tool("prop:tree")
	editor._click(Vector2(2.0, 3.0), false)
	check(not editor.undo_button.disabled, "adding a tree can be undone")
	editor.undo()
	check(editor._place()["props"].size() == props_before, "Undo removes the tree")
	check(editor.selected == {"kind": "place", "id": "village"}, "the missing tree's selection falls back to the place")
	check(editor.undo_button.disabled and not editor.redo_button.disabled, "then only Redo is possible")
	editor.redo()
	check(editor._place()["props"].size() == props_before + 1, "Redo brings the tree back")
	check(editor.dirty, "undo and redo mark the story unsaved")

	editor._click(Vector2(-2.0, -0.5), false)
	var mayor_pos: Array = StoryData.find(editor.story, "characters", "mayor")["pos"].duplicate()
	for x in [-3.0, -4.0, -5.0]:
		editor._drag_to(Vector2(x, 1.0))
	var release := InputEventMouseButton.new()
	release.button_index = MOUSE_BUTTON_LEFT
	editor._on_view_input(release)
	var steps: int = editor._undo.size()
	editor.undo()
	check(StoryData.find(editor.story, "characters", "mayor")["pos"] == mayor_pos, "one Undo puts back a whole drag")
	check(editor.selected == {"kind": "character", "id": "mayor"}, "the mayor stays selected")
	check(editor._undo.size() == steps - 1, "a drag is one step")

	editor._select("story", "")
	await _frames(2)
	var title_before: String = editor.story["title"]
	var title_edit: LineEdit = editor.inspector.find_children("*", "LineEdit", true, false)[0]
	steps = editor._undo.size()
	for text in ["R", "Ra", "Rain"]:
		title_edit.text_changed.emit(text)
	check(editor.story["title"] == "Rain" and editor._undo.size() == steps + 1, "typing a title is one step")
	check(editor._redo.is_empty(), "a new edit clears Redo")
	title_edit.grab_focus()
	editor._shortcut_input(_ctrl_key(KEY_Z))
	check(editor.story["title"] == "Rain", "Ctrl+Z is left to a focused text field")
	title_edit.release_focus()
	editor._shortcut_input(_ctrl_key(KEY_Z))
	check(editor.story["title"] == title_before, "Ctrl+Z undoes the whole title at once")
	editor._shortcut_input(_ctrl_key(KEY_Z, true))
	check(editor.story["title"] == "Rain", "Ctrl+Shift+Z redoes it")
	editor._shortcut_input(_ctrl_key(KEY_Z))
	editor._shortcut_input(_ctrl_key(KEY_Y))
	check(editor.story["title"] == "Rain", "so does Ctrl+Y")

	editor._go_to("item", "notebook")
	editor._add_place()
	check(editor._place()["name"] == "New place", "adding a place shows it")
	editor.undo()
	check(editor.place_id == editor.story["start_place"], "undoing the place goes back to the start place")
	await _frames(2)
	check(editor.outline.get_child_count() > 0 and editor.inspector.get_child_count() > 0, "the outline and inspector rebuild")

	editor.load_story(sample(), SAMPLE)
	check(editor._undo.is_empty() and editor._redo.is_empty(), "loading a story clears the history")
	check(editor.undo_button.disabled and editor.redo_button.disabled, "and disables Undo and Redo")
	editor.queue_free()
	await _frames(1)


## Starts play mode in a place made of `props`, with the intro card closed.
func _play_in(props: Array, start: Array):
	var story := StoryData.normalized({"places": [{"id": "a", "size": 30, "props": props}],
			"start_place": "a", "start_pos": start})
	var play = PlayMode.new()
	play.setup(story)
	root.add_child(play)
	await _frames(3)
	play._close_card()
	await _physics(10)
	return play


func _physics(n: int) -> void:
	for i in n:
		await physics_frame


## Presses and releases a key, going through the viewport like a real one.
func _key(keycode: Key) -> void:
	for pressed in [true, false]:
		var event := InputEventKey.new()
		event.keycode = keycode
		event.physical_keycode = keycode
		event.pressed = pressed
		root.push_input(event)


## Holds a move action (or none) for `n` physics frames and returns the
## highest the student got.
func _hold(action: String, n: int, player) -> float:
	var top := -INF
	if action != "":
		Input.action_press(action)
	for i in n:
		await physics_frame
		top = maxf(top, player.global_position.y)
	if action != "":
		Input.action_release(action)
	return top


func test_jumping() -> void:
	var play = await _play_in([{"kind": "crate", "pos": [0, -1.5], "rot": 0}], [0, 0.4])
	var player = play.player
	check(player.is_on_floor(), "the student starts on the floor")
	_key(KEY_SPACE)
	await _physics(5)
	check(player.visual.airborne, "the actor knows the student is in the air")
	var top: float = await _hold("", 60, player)
	check(top > 0.85 and top < 1.3, "Space jumps about a metre (%.2f)" % top)
	check(player.is_on_floor() and player.global_position.y < 0.1, "the student lands again (y = %.2f)" % player.global_position.y)

	_key(KEY_SPACE)
	var stood_on := -1.0
	Input.action_press("move_forward")
	for i in 40:
		await physics_frame
		if player.is_on_floor() and player.global_position.y > stood_on:
			stood_on = player.global_position.y
	Input.action_release("move_forward")
	check(absf(stood_on - 0.9) < 0.1, "a jump gets onto a crate (stood at y = %.2f)" % stood_on)
	play.queue_free()
	await _frames(2)

	play = await _play_in([{"kind": "river", "pos": [0, 0], "rot": 0}], [-2.2, 0])
	var splashes := [0]
	play.player.splashed.connect(func(): splashes[0] += 1)
	_key(KEY_SPACE)
	await _hold("move_right", 60, play.player)
	check(splashes[0] > 0 and play.player.global_position.x < -1.5, "jumping onto a river puts the student back on the bank (x = %.2f)" % play.player.global_position.x)
	play.queue_free()
	await _frames(2)


func test_space_still_talks_and_confirms() -> void:
	var play = PlayMode.new()
	play.setup(sample())
	root.add_child(play)
	await _frames(3)
	check(play.overlay.visible, "the intro card shows")
	_key(KEY_SPACE)
	await _frames(1)
	check(not play.overlay.visible, "Space still presses the card's button")
	var top: float = await _hold("", 20, play.player)
	check(top < 0.1, "closing the card with Space doesn't jump (%.2f)" % top)

	var mayor: Dictionary = play.place_node.get_meta("interactables").filter(func(e): return e["id"] is String and e["id"] == "mayor")[0]
	play._activate(mayor)
	await _frames(2)
	check(play.dialogue.typing(), "the mayor starts talking")
	_key(KEY_SPACE)
	check(not play.dialogue.typing(), "Space finishes the line")
	top = await _hold("", 30, play.player)
	check(top < 0.1, "the student doesn't jump while talking (%.2f)" % top)
	play.player.jump()
	await _physics(5)
	check(play.player.global_position.y < 0.1, "jump() does nothing while frozen")
	play._end_talk()
	await _frames(1)
	play.queue_free()
	await _frames(2)


func test_sitting_on_benches() -> void:
	# The bench turned 90 degrees faces +x; the other two stand side by side facing +z.
	var props := [{"kind": "bench", "pos": [2, 0], "rot": 90}, {"kind": "bench", "pos": [-2, 0], "rot": 0},
			{"kind": "bench", "pos": [0, 0], "rot": 0}]
	var play = await _play_in(props, [3.2, 0])
	var player = play.player
	await _frames(2)
	check(play.focus.get("kind") == "seat" and play.focus["id"] == 0, "the turned bench is in reach")
	check(play.prompt.visible and play.prompt.text.ends_with("Sit on the bench"), "the prompt offers a seat: %s" % play.prompt.text)
	_key(KEY_E)
	check(player.seat == play.focus["node"], "E sits down")
	var at: Vector3 = player.global_position
	check(at.distance_to(Vector3(2.14, 0.55, 0)) < 0.05, "the student is on the seat: %s" % at)
	check(play.prompt.text.ends_with("Stand up"), "the prompt offers standing up")
	await create_timer(0.6).timeout
	check(absf(angle_difference(player.visual.rotation.y, PI / 2.0)) < 0.1, "the student faces the way the bench does (%.2f)" % player.visual.rotation.y)
	check(player.visual.get_node("Legs").visible, "the legs show while sitting")
	await _physics(10)
	check(player.global_position.distance_to(at) < 0.01, "sitting stays put")

	await _hold("move_back", 2, player)
	check(player.seat == null and not player.visual.sitting, "moving stands the student up")
	check(not player.visual.get_node("Legs").visible, "the legs hide again")
	check(player.global_position.x > 2.8 and player.global_position.y < 0.3, "standing up puts the student in front of the bench: %s" % player.global_position)
	await _frames(2)
	check(play.prompt.text.ends_with("Sit on the bench"), "the prompt offers the seat again")

	_key(KEY_E)
	check(player.seat != null, "E sits down again")
	_key(KEY_E)
	check(player.seat == null, "and E stands up")
	_key(KEY_E)
	_key(KEY_SPACE)
	var top: float = await _hold("", 20, player)
	check(player.seat == null and top < 0.2, "Space stands up instead of jumping (%.2f)" % top)

	player.global_position = Vector3(0, 0.05, 1.2)
	await _physics(5)
	await _frames(2)
	check(play.focus.get("id") == 2, "of two benches side by side, the nearer one is in reach")
	_key(KEY_E)
	check(player.global_position.distance_to(Vector3(0, 0.55, 0.14)) < 0.05, "and that is the one sat on: %s" % player.global_position)
	play.queue_free()
	await _frames(2)
