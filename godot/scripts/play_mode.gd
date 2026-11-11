extends Node
## Plays a story: the student walks around, talks to people, picks things up
## and goes through doors until a choice ends the story.

signal exited

const StoryData = preload("res://scripts/story.gd")
const StoryState = preload("res://scripts/story_state.gd")
const Builder = preload("res://scripts/world_builder.gd")
const Player = preload("res://scripts/player.gd")
const CameraRig = preload("res://scripts/camera_rig.gd")
const DialogueBox = preload("res://scripts/dialogue_box.gd")
const TouchStick = preload("res://scripts/touch_stick.gd")

## True on phones and tablets, and anywhere once the screen has been touched.
## Kept between stories, so the joystick stays after "Play again".
static var touch := OS.has_feature("mobile") or OS.has_feature("web_android") or OS.has_feature("web_ios")

var story: Dictionary
var exit_text := "Exit"
var state
var place_node: Node3D
var player
var rig
## The thing the student is close enough to use, or {}.
var focus := {}
## The character being talked to, or {}.
var speaking := {}
var line := -1
## True while a card, conversation or door transition is in the way.
var busy := true

var goal_label: Label
var backpack: HFlowContainer
var prompt: Button
var exit_button: Button
var stick: Control
var toasts: VBoxContainer
var dialogue
var overlay: Control
var card_box: VBoxContainer
var fade: ColorRect


func setup(s: Dictionary, exit_button_text := "Exit") -> void:
	story = StoryData.normalized(s)
	exit_text = exit_button_text


func _ready() -> void:
	player = Player.new()
	add_child(player)
	rig = CameraRig.new()
	add_child(rig)
	rig.target = player
	rig.exclude(player)
	player.rig = rig
	_build_hud()
	if touch:
		_use_touch()
	_start()


func _start() -> void:
	state = StoryState.new(story)
	_refresh_backpack()
	_enter_place()
	var intro: String = story["intro"] if story["intro"] != "" else "Explore, talk to people and see where the story takes you."
	_show_card(story["title"], intro, [["Begin", _close_card]])


func _enter_place() -> void:
	if place_node:
		remove_child(place_node)
		place_node.queue_free()
	place_node = Builder.build_place(story, state.place, state.collected)
	add_child(place_node)
	player.global_position = Vector3(state.pos[0], 0.05, state.pos[1])
	player.velocity = Vector3.ZERO
	rig.snap()
	focus = {}
	_update_prompt()
	var place := StoryData.find(story, "places", state.place)
	_toast(place.get("name", ""))


func _process(_delta: float) -> void:
	stick.visible = touch and not busy
	if place_node:
		Builder.update_walls(place_node, rig.camera.global_position)
	if busy or place_node == null:
		return
	var best := {}
	var best_distance := INF
	for entry in place_node.get_meta("interactables"):
		var node: Node3D = entry["node"]
		if not is_instance_valid(node) or not node.is_inside_tree():
			continue
		var offset: Vector3 = node.global_position - player.global_position
		var d := Vector2(offset.x, offset.z).length()
		if d < entry["radius"] and d < best_distance:
			best = entry
			best_distance = d
	if best.get("node") != focus.get("node"):
		focus = best
		_update_prompt()


func _input(event: InputEvent) -> void:
	if event is InputEventScreenTouch and not touch:
		touch = true
		_use_touch()


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("ui_cancel"):
		if not speaking.is_empty():
			_end_talk()
		elif not overlay.visible:
			exited.emit()
		get_viewport().set_input_as_handled()
		return
	if dialogue.visible:
		if event is InputEventKey and event.pressed and not event.echo:
			if event.keycode >= KEY_1 and event.keycode <= KEY_9:
				dialogue.pick(event.keycode - KEY_1)
				get_viewport().set_input_as_handled()
			elif event.is_action("interact"):
				dialogue.advance()
				get_viewport().set_input_as_handled()
		return
	if not busy and not focus.is_empty() and event.is_action_pressed("interact"):
		get_viewport().set_input_as_handled()
		_activate(focus)


func _activate(entry: Dictionary) -> void:
	match entry["kind"]:
		"character":
			_talk(entry)
		"item":
			_pick_up(entry)
		"door":
			_go_through(entry)


# Talking ---------------------------------------------------------------------

func _talk(entry: Dictionary) -> void:
	var character := StoryData.find(story, "characters", entry["id"])
	var start: int = state.opening_line(character)
	if start < 0:
		_toast("%s has nothing to say right now." % character["name"])
		return
	busy = true
	player.frozen = true
	speaking = character
	var node = entry["node"]
	node.face(player.global_position)
	node.talking = true
	player.visual.face(node.global_position)
	prompt.visible = false
	_say(start)


func _say(index: int) -> void:
	line = index
	var options: Array = state.choices(speaking, index)
	if options.is_empty():
		options = [{"index": -1, "text": "Goodbye."}]
	dialogue.show_line(speaking["name"], Color(speaking["color"]), speaking["lines"][index]["text"], options)


func _on_chosen(index: int) -> void:
	if index < 0:
		_end_talk()
		return
	var before: Array = state.inventory.duplicate()
	var next: int = state.choose(speaking, line, index)
	_report_inventory(before)
	if state.ending != "":
		_end_talk()
		_show_ending()
	elif next < 0:
		_end_talk()
	else:
		_say(next)


func _end_talk() -> void:
	dialogue.close()
	var node = place_node.get_meta("entities").get("character:" + str(speaking.get("id", "")))
	if node:
		node.talking = false
	speaking = {}
	player.frozen = false
	busy = false
	_remove_collected()
	_update_prompt()


# Items and doors -------------------------------------------------------------

func _pick_up(entry: Dictionary) -> void:
	var before: Array = state.inventory.duplicate()
	state.pick_up(entry["id"])
	_report_inventory(before)
	place_node.get_meta("interactables").erase(entry)
	place_node.get_meta("entities").erase("item:" + entry["id"])
	var node: Node3D = entry["node"]
	var tween := create_tween()
	tween.tween_property(node, "scale", Vector3.ONE * 1.5, 0.12)
	tween.tween_property(node, "scale", Vector3.ONE * 0.01, 0.2)
	tween.tween_callback(node.queue_free)
	focus = {}
	_update_prompt()
	var item: Dictionary = state.item(entry["id"])
	if item["description"] != "":
		_show_card("You found: %s" % item["name"], item["description"], [["OK", _close_card]])


func _go_through(entry: Dictionary) -> void:
	if StoryData.find(story, "places", entry["to"]).is_empty():
		_toast("This door doesn't lead anywhere yet.")
		return
	busy = true
	player.frozen = true
	prompt.visible = false
	var tween := create_tween()
	tween.tween_property(fade, "color:a", 1.0, 0.25)
	tween.tween_callback(_travel.bind(entry["to"]))
	tween.tween_property(fade, "color:a", 0.0, 0.3)
	tween.tween_callback(_resume)


func _travel(to_place: String) -> void:
	state.travel(to_place)
	_enter_place()


func _resume() -> void:
	busy = false
	player.frozen = false
	_update_prompt()


func _remove_collected() -> void:
	var entities: Dictionary = place_node.get_meta("entities")
	var interactables: Array = place_node.get_meta("interactables")
	for entry in interactables.duplicate():
		if entry["kind"] == "item" and state.collected.has(entry["id"]):
			interactables.erase(entry)
			entities.erase("item:" + entry["id"])
			entry["node"].queue_free()


func _report_inventory(before: Array) -> void:
	for id in state.inventory:
		if not before.has(id):
			_toast("Added to your backpack: %s" % state.item(id).get("name", id))
	for id in before:
		if not state.inventory.has(id):
			_toast("You handed over: %s" % state.item(id).get("name", id))
	_refresh_backpack()


func _show_ending() -> void:
	busy = true
	player.frozen = true
	_show_card("The end", state.ending, [["Play again", _restart], [exit_text, func(): exited.emit()]])


func _restart() -> void:
	_close_card()
	_start()


# HUD -------------------------------------------------------------------------

func _build_hud() -> void:
	var layer := CanvasLayer.new()
	add_child(layer)

	var margin := MarginContainer.new()
	_full_rect(margin)
	for side in ["left", "right", "top", "bottom"]:
		margin.add_theme_constant_override("margin_" + side, 20)
	layer.add_child(margin)
	var column := VBoxContainer.new()
	_ignore(column)
	margin.add_child(column)

	var top := HBoxContainer.new()
	_ignore(top)
	column.add_child(top)
	var goal_panel := PanelContainer.new()
	goal_panel.theme_type_variation = "Card"
	goal_panel.custom_minimum_size.x = 340
	goal_panel.size_flags_horizontal = Control.SIZE_SHRINK_BEGIN
	goal_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	top.add_child(goal_panel)
	var goal_box := VBoxContainer.new()
	_ignore(goal_box)
	goal_panel.add_child(goal_box)
	var title := Label.new()
	title.text = story["title"]
	title.theme_type_variation = "HeaderLabel"
	goal_box.add_child(title)
	goal_label = Label.new()
	goal_label.text = story["goal"]
	goal_label.visible = story["goal"] != ""
	goal_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	goal_label.custom_minimum_size.x = 320
	goal_box.add_child(goal_label)
	var backpack_label := Label.new()
	backpack_label.text = "BACKPACK"
	backpack_label.theme_type_variation = "SectionLabel"
	goal_box.add_child(backpack_label)
	backpack = HFlowContainer.new()
	_ignore(backpack)
	goal_box.add_child(backpack)

	top.add_child(_spacer(true, false))
	exit_button = Button.new()
	exit_button.text = exit_text
	exit_button.focus_mode = Control.FOCUS_NONE
	exit_button.size_flags_vertical = Control.SIZE_SHRINK_BEGIN
	exit_button.pressed.connect(func(): exited.emit())
	top.add_child(exit_button)

	column.add_child(_spacer(false, true))
	var bottom := VBoxContainer.new()
	_ignore(bottom)
	bottom.alignment = BoxContainer.ALIGNMENT_END
	bottom.add_theme_constant_override("separation", 12)
	column.add_child(bottom)
	prompt = Button.new()
	prompt.theme_type_variation = "AccentButton"
	prompt.focus_mode = Control.FOCUS_NONE
	prompt.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	prompt.visible = false
	prompt.pressed.connect(func():
		if not focus.is_empty():
			_activate(focus)
	)
	bottom.add_child(prompt)
	dialogue = DialogueBox.new()
	dialogue.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	dialogue.chosen.connect(_on_chosen)
	bottom.add_child(dialogue)

	var toast_margin := MarginContainer.new()
	_full_rect(toast_margin)
	toast_margin.add_theme_constant_override("margin_top", 24)
	layer.add_child(toast_margin)
	toasts = VBoxContainer.new()
	_ignore(toasts)
	toasts.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	toasts.size_flags_vertical = Control.SIZE_SHRINK_BEGIN
	toast_margin.add_child(toasts)

	stick = TouchStick.new()
	stick.visible = false
	stick.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_LEFT, Control.PRESET_MODE_KEEP_SIZE, 30)
	layer.add_child(stick)

	overlay = ColorRect.new()
	overlay.color = Color(0.1, 0.08, 0.05, 0.45)
	overlay.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	overlay.visible = false
	layer.add_child(overlay)
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	overlay.add_child(center)
	var card := PanelContainer.new()
	card.theme_type_variation = "Card"
	card.custom_minimum_size = Vector2(600, 0)
	center.add_child(card)
	card_box = VBoxContainer.new()
	card_box.add_theme_constant_override("separation", 16)
	card.add_child(card_box)

	fade = ColorRect.new()
	fade.color = Color(0, 0, 0, 0)
	fade.mouse_filter = Control.MOUSE_FILTER_IGNORE
	fade.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	layer.add_child(fade)


func _show_card(title: String, text: String, buttons: Array) -> void:
	busy = true
	player.frozen = true
	for child in card_box.get_children():
		card_box.remove_child(child)
		child.queue_free()
	var heading := Label.new()
	heading.text = title
	heading.theme_type_variation = "HeaderLabel"
	heading.add_theme_font_size_override("font_size", 30)
	card_box.add_child(heading)
	var body := Label.new()
	body.text = text
	body.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	body.custom_minimum_size.x = 560
	body.add_theme_font_size_override("font_size", 19)
	card_box.add_child(body)
	var row := HBoxContainer.new()
	row.alignment = BoxContainer.ALIGNMENT_END
	row.add_theme_constant_override("separation", 10)
	card_box.add_child(row)
	for i in buttons.size():
		var button := Button.new()
		button.text = buttons[i][0]
		if i == 0:
			button.theme_type_variation = "AccentButton"
		if touch:
			button.custom_minimum_size = Vector2(150, 64)
		button.pressed.connect(buttons[i][1])
		row.add_child(button)
	overlay.visible = true
	var first: Button = row.get_child(0)
	first.grab_focus.call_deferred()


func _close_card() -> void:
	overlay.visible = false
	busy = false
	player.frozen = false
	_update_prompt()


func _update_prompt() -> void:
	if prompt == null:
		return
	prompt.visible = not busy and not focus.is_empty()
	if prompt.visible:
		prompt.text = focus["label"] if touch else "E   %s" % focus["label"]


## Shows the joystick and makes the buttons big enough to tap.
func _use_touch() -> void:
	prompt.custom_minimum_size = Vector2(320, 88)
	prompt.add_theme_font_size_override("font_size", 26)
	exit_button.custom_minimum_size = Vector2(120, 60)
	dialogue.touch = true
	stick.visible = not busy
	_update_prompt()


func _refresh_backpack() -> void:
	for child in backpack.get_children():
		backpack.remove_child(child)
		child.queue_free()
	if state.inventory.is_empty():
		var empty := Label.new()
		empty.text = "Empty"
		empty.theme_type_variation = "MutedLabel"
		backpack.add_child(empty)
	for id in state.inventory:
		var item: Dictionary = state.item(id)
		var chip := PanelContainer.new()
		chip.theme_type_variation = "Chip"
		chip.tooltip_text = item.get("description", "")
		var row := HBoxContainer.new()
		chip.add_child(row)
		var dot := ColorRect.new()
		dot.color = Color(item.get("color", "#f2c14e"))
		dot.custom_minimum_size = Vector2(12, 12)
		dot.size_flags_vertical = Control.SIZE_SHRINK_CENTER
		row.add_child(dot)
		var name_label := Label.new()
		name_label.text = item.get("name", id)
		row.add_child(name_label)
		backpack.add_child(chip)


func _toast(text: String) -> void:
	if text == "" or toasts == null:
		return
	var chip := PanelContainer.new()
	chip.theme_type_variation = "Chip"
	chip.mouse_filter = Control.MOUSE_FILTER_IGNORE
	chip.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	var label := Label.new()
	label.text = text
	chip.add_child(label)
	toasts.add_child(chip)
	chip.modulate.a = 0.0
	var tween := chip.create_tween()
	tween.tween_property(chip, "modulate:a", 1.0, 0.2)
	tween.tween_interval(2.6)
	tween.tween_property(chip, "modulate:a", 0.0, 0.4)
	tween.tween_callback(chip.queue_free)


func _full_rect(control: Control) -> void:
	control.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	control.mouse_filter = Control.MOUSE_FILTER_IGNORE


func _ignore(control: Control) -> void:
	control.mouse_filter = Control.MOUSE_FILTER_IGNORE


func _spacer(horizontal: bool, vertical: bool) -> Control:
	var spacer := Control.new()
	_ignore(spacer)
	if horizontal:
		spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	if vertical:
		spacer.size_flags_vertical = Control.SIZE_EXPAND_FILL
	return spacer
