extends Node
## Switches between the menu, the editor and play mode.
##
## Command-line options (after `--`):
##   --play <story.json>      play a story straight away
##   --edit <story.json>      open a story in the editor
##   --screenshot <out.png>   save a screenshot after --wait seconds (default 2), then quit
##
## In a browser, a link ending in #story=... plays the story packed into it.

const StoryData = preload("res://scripts/story.gd")
const Controls = preload("res://scripts/controls.gd")
const UITheme = preload("res://scripts/ui_theme.gd")
const Builder = preload("res://scripts/world_builder.gd")
const CameraRig = preload("res://scripts/camera_rig.gd")
const PlayMode = preload("res://scripts/play_mode.gd")
const StoryEditor = preload("res://scripts/editor_mode.gd")
const StoryFiles = preload("res://scripts/story_files.gd")

const SAMPLE := "res://stories/water_cycle.json"

var screen: Node
var editor: Node
var files: StoryFiles
var _open_to := "play"
var message: Label


func _ready() -> void:
	Controls.ensure_actions()
	files = StoryFiles.new()
	files.opened.connect(_open_story)
	files.failed.connect(_show_error)
	add_child(files)

	var args := OS.get_cmdline_user_args()
	var play_path := _arg(args, "--play")
	var edit_path := _arg(args, "--edit")
	var linked := StoryFiles.story_in_link()
	if linked.has("story"):
		_play(linked["story"])
	elif linked.has("error"):
		_show_menu()
		_show_error(linked["error"])
	elif play_path != "":
		_open_to = "play"
		_open_file(play_path)
	elif edit_path != "":
		_open_to = "edit"
		_open_file(edit_path)
	else:
		_show_menu()
	var shot := _arg(args, "--screenshot")
	if shot != "":
		var wait := float(_arg(args, "--wait", "2"))
		await get_tree().create_timer(wait).timeout
		await RenderingServer.frame_post_draw
		get_viewport().get_texture().get_image().save_png(shot)
		get_tree().quit()


func _arg(args: PackedStringArray, name: String, fallback := "") -> String:
	var i := args.find(name)
	return args[i + 1] if i >= 0 and i + 1 < args.size() else fallback


func _set_screen(node: Node, keep_old := false) -> void:
	if screen:
		remove_child(screen)
		if not keep_old:
			screen.queue_free()
	screen = node
	add_child(node)
	UITheme.apply(node)


# Menu ------------------------------------------------------------------------

func _show_menu() -> void:
	editor = null
	var menu := Node.new()
	var sample := StoryData.load_file(SAMPLE)
	if sample.has("story"):
		var story: Dictionary = sample["story"]
		menu.add_child(Builder.build_place(story, story["start_place"]))
	var rig := CameraRig.new()
	rig.listen = false
	rig.auto_orbit = 0.06
	rig.distance = 24.0
	rig.pitch = deg_to_rad(-22.0)
	menu.add_child(rig)

	var layer := CanvasLayer.new()
	menu.add_child(layer)
	var margin := MarginContainer.new()
	margin.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	for side in ["left", "right", "top", "bottom"]:
		margin.add_theme_constant_override("margin_" + side, 56)
	layer.add_child(margin)
	var row := HBoxContainer.new()
	margin.add_child(row)
	var card := PanelContainer.new()
	card.theme_type_variation = "Card"
	card.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	card.custom_minimum_size.x = 440
	row.add_child(card)
	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 12)
	card.add_child(box)
	var title := Label.new()
	title.text = "Educore Stories"
	title.theme_type_variation = "TitleLabel"
	box.add_child(title)
	var tagline := Label.new()
	tagline.text = "Make story games for your class: places to explore, people to talk to and things to find."
	tagline.theme_type_variation = "MutedLabel"
	tagline.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	box.add_child(tagline)
	box.add_child(_menu_button("Play the sample story", _play_sample, true))
	var draft := StoryFiles.load_draft()
	if not draft.is_empty():
		box.add_child(_menu_button("Continue editing \"%s\"" % draft["title"], func(): _edit(draft, "")))
	box.add_child(_menu_button("Create a new story", func(): _edit(StoryData.blank(), "")))
	box.add_child(_menu_button("Edit the sample story", _edit_sample))
	box.add_child(_menu_button("Open a story to edit...", _ask_open.bind("edit")))
	box.add_child(_menu_button("Open a story to play...", _ask_open.bind("play")))
	if not StoryFiles.on_web():
		box.add_child(_menu_button("Quit", func(): get_tree().quit()))
	message = Label.new()
	message.theme_type_variation = "WarningLabel"
	message.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	message.visible = false
	box.add_child(message)
	_set_screen(menu)


func _menu_button(text: String, on_press: Callable, accent := false) -> Button:
	var button := Button.new()
	button.text = text
	button.alignment = HORIZONTAL_ALIGNMENT_LEFT
	if accent:
		button.theme_type_variation = "AccentButton"
	button.pressed.connect(on_press)
	return button


func _play_sample() -> void:
	_open_to = "play"
	_open_file(SAMPLE)


func _edit_sample() -> void:
	_open_to = "edit"
	_open_file(SAMPLE)


func _ask_open(mode: String) -> void:
	_open_to = mode
	files.ask_open()


func _open_file(file_path: String) -> void:
	var result := StoryData.load_file(file_path)
	if result.has("error"):
		_show_error(result["error"])
	else:
		_open_story(result["story"], file_path)


func _open_story(story: Dictionary, file_path: String) -> void:
	if _open_to == "edit":
		_edit(story, file_path)
	else:
		_play(story)


func _show_error(text: String) -> void:
	if screen == null:
		_show_menu()
	if is_instance_valid(message):
		message.text = text
		message.visible = true
	push_warning(text)


# Play and edit ---------------------------------------------------------------

func _play(story: Dictionary) -> void:
	var play := PlayMode.new()
	play.setup(story, "Menu")
	play.exited.connect(_show_menu, CONNECT_DEFERRED)
	_set_screen(play)


func _edit(story: Dictionary, file_path: String) -> void:
	editor = StoryEditor.new()
	editor.load_story(story, file_path)
	editor.play_requested.connect(_playtest, CONNECT_DEFERRED)
	editor.exit_requested.connect(_show_menu, CONNECT_DEFERRED)
	_set_screen(editor)


## Plays the story being edited, then returns to the editor as it was.
func _playtest(story: Dictionary) -> void:
	var play := PlayMode.new()
	play.setup(story, "Back to editor")
	play.exited.connect(func(): _set_screen(editor), CONNECT_DEFERRED)
	_set_screen(play, true)
