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
	]:
		current = test
		call(test)
	current = "test_play_mode_talk_pick_up_and_travel"
	await test_play_mode_talk_pick_up_and_travel()
	current = "test_editor_places_moves_and_deletes"
	await test_editor_places_moves_and_deletes()
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
	check(not village.get_meta("entities").has("start"), "the start marker only shows while editing")
	village.free()
	var library := Builder.build_place(story, "library", {"notebook": true})
	check(not library.get_meta("entities").has("item:notebook"), "collected items stay gone")
	library.free()


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
