extends Control
## The teacher's story editor: the story's places, people and items on the
## left, a 3D view of the current place in the middle, and the selected
## thing's settings (including dialogue) on the right.

signal play_requested(story: Dictionary)
signal exit_requested

const StoryData = preload("res://scripts/story.gd")
const Builder = preload("res://scripts/world_builder.gd")
const CameraRig = preload("res://scripts/camera_rig.gd")
const StoryFiles = preload("res://scripts/story_files.gd")

const GRID := 0.5
const SELECT_COLOR := Color("#ffb347")
## In a browser, how often (in seconds) the story is kept as a draft.
const DRAFT_EVERY := 1.0
## How many steps Undo can go back.
const HISTORY := 100
## Edits to the same field closer together than this (in ms) undo as one step.
const BURST_MS := 1000

var story: Dictionary
var path := ""
var dirty := false
var place_id := ""
## {"kind": "story" | "place" | "prop" | "door" | "character" | "item", "id": ...}
var selected := {"kind": "story", "id": ""}
var tool := "select"
var dragging := ""

var title_label: Label
var status_label: Label
var outline: VBoxContainer
var inspector: VBoxContainer
var place_picker: OptionButton
var tool_buttons := {}
var view: SubViewportContainer
var viewport: SubViewport
var place_node: Node3D
var rig
var hint: Label
var files: StoryFiles
var confirm: ConfirmationDialog
var _on_confirm := Callable()
var _ring: Node3D
var _drag_moved := false
var _outline_pending := false
var _inspector_pending := false
var _draft_pending := false
var _draft_timer := 0.0
var undo_button: Button
var redo_button: Button
## Earlier and undone versions of the story, oldest first.
var _undo := []
var _redo := []
## The story as of the last recorded step.
var _last := {}
## The [entry, key] last edited, and when, so typing counts as one step.
var _field := []
var _field_time := 0


func _init() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_build()
	load_story(StoryData.blank(), "")


func load_story(s: Dictionary, file_path: String) -> void:
	story = StoryData.normalized(s)
	path = file_path
	dirty = false
	tool = "select"
	_show_place(story["start_place"])
	_select("story", "")
	_update_title()
	_undo.clear()
	_redo.clear()
	_last = story.duplicate(true)
	_field = []
	_update_history_buttons()


func _shortcut_input(event: InputEvent) -> void:
	if not (event is InputEventKey and event.pressed and event.is_command_or_control_pressed()):
		return
	var focus := get_viewport().gui_get_focus_owner()
	if focus is LineEdit or focus is TextEdit:
		return
	if event.keycode == KEY_Z and not event.shift_pressed:
		undo()
	elif event.keycode == KEY_Z or event.keycode == KEY_Y:
		redo()
	else:
		return
	get_viewport().set_input_as_handled()


func _process(delta: float) -> void:
	_draft_timer += delta
	if _draft_timer >= DRAFT_EVERY:
		_save_draft()
	rig.pan_keys = view.has_focus()
	Builder.update_walls(place_node, rig.camera.global_position)
	if _outline_pending:
		_outline_pending = false
		_rebuild_outline()
	if _inspector_pending:
		_inspector_pending = false
		_rebuild_inspector()


# Layout ----------------------------------------------------------------------

func _build() -> void:
	var background := ColorRect.new()
	background.color = Color("#efe8dc")
	background.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(background)
	var root := VBoxContainer.new()
	root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	root.add_theme_constant_override("separation", 0)
	add_child(root)

	var bar_panel := PanelContainer.new()
	bar_panel.theme_type_variation = "Bar"
	root.add_child(bar_panel)
	var bar := HBoxContainer.new()
	bar.add_theme_constant_override("separation", 8)
	bar_panel.add_child(bar)
	title_label = Label.new()
	title_label.theme_type_variation = "HeaderLabel"
	bar.add_child(title_label)
	status_label = Label.new()
	status_label.theme_type_variation = "MutedLabel"
	status_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	status_label.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	bar.add_child(status_label)
	bar.add_child(_button("New", _on_new))
	bar.add_child(_button("Open...", _on_open))
	undo_button = _button("Undo", undo)
	bar.add_child(undo_button)
	redo_button = _button("Redo", redo)
	bar.add_child(redo_button)
	if StoryFiles.on_web():
		bar.add_child(_button("Download", _on_save))
		bar.add_child(_button("Copy student link", _on_copy_link))
	else:
		bar.add_child(_button("Save", _on_save))
		bar.add_child(_button("Save as...", _on_save_as))
	var play := _button("Play", _on_play)
	play.theme_type_variation = "AccentButton"
	play.custom_minimum_size.x = 110
	bar.add_child(play)
	bar.add_child(_button("Exit", _on_exit))

	var body := HBoxContainer.new()
	body.size_flags_vertical = Control.SIZE_EXPAND_FILL
	body.add_theme_constant_override("separation", 0)
	root.add_child(body)

	outline = VBoxContainer.new()
	body.add_child(_sidebar(outline, 250))

	var center := VBoxContainer.new()
	center.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	center.add_theme_constant_override("separation", 0)
	body.add_child(center)
	var tools_panel := PanelContainer.new()
	tools_panel.theme_type_variation = "Bar"
	center.add_child(tools_panel)
	var tools_box := VBoxContainer.new()
	tools_panel.add_child(tools_box)
	var place_row := HBoxContainer.new()
	tools_box.add_child(place_row)
	var place_label := Label.new()
	place_label.text = "Place"
	place_label.theme_type_variation = "FieldLabel"
	place_row.add_child(place_label)
	place_picker = OptionButton.new()
	place_picker.custom_minimum_size.x = 220
	place_picker.item_selected.connect(func(i): _show_place(story["places"][i]["id"]))
	place_row.add_child(place_picker)
	var tools := HFlowContainer.new()
	tools_box.add_child(tools)
	var group := ButtonGroup.new()
	var tool_specs := [["select", "Select"], ["character", "Person"], ["item", "Item"], ["door", "Door"]]
	for kind in StoryData.PROP_KINDS:
		tool_specs.append(["prop:" + kind, kind.capitalize()])
	tool_specs.append(["start", "Start point"])
	for spec in tool_specs:
		var button := Button.new()
		button.text = spec[1]
		button.toggle_mode = true
		button.button_group = group
		button.theme_type_variation = "SmallButton"
		button.focus_mode = Control.FOCUS_NONE
		button.pressed.connect(_set_tool.bind(spec[0]))
		tools.add_child(button)
		tool_buttons[spec[0]] = button

	view = SubViewportContainer.new()
	view.stretch = true
	view.size_flags_vertical = Control.SIZE_EXPAND_FILL
	view.focus_mode = Control.FOCUS_ALL
	view.gui_input.connect(_on_view_input)
	center.add_child(view)
	viewport = SubViewport.new()
	viewport.own_world_3d = true
	viewport.msaa_3d = Viewport.MSAA_4X
	view.add_child(viewport)
	rig = CameraRig.new()
	rig.listen = false
	rig.distance = 22.0
	rig.pitch = deg_to_rad(-50.0)
	viewport.add_child(rig)
	var hint_panel := PanelContainer.new()
	hint_panel.theme_type_variation = "Bar"
	center.add_child(hint_panel)
	hint = Label.new()
	hint.theme_type_variation = "MutedLabel"
	hint.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	hint_panel.add_child(hint)

	inspector = VBoxContainer.new()
	inspector.add_theme_constant_override("separation", 10)
	body.add_child(_sidebar(inspector, 400))

	files = StoryFiles.new()
	files.opened.connect(load_story)
	files.saved.connect(_on_saved)
	files.failed.connect(_status)
	add_child(files)
	confirm = ConfirmationDialog.new()
	confirm.confirmed.connect(func(): _on_confirm.call())
	add_child(confirm)
	_set_tool("select")


func _sidebar(content: VBoxContainer, width: float) -> Control:
	var panel := PanelContainer.new()
	panel.theme_type_variation = "Sidebar"
	panel.custom_minimum_size.x = width
	var scroll := ScrollContainer.new()
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	panel.add_child(scroll)
	content.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.add_child(content)
	return panel


# The 3D view -----------------------------------------------------------------

func _show_place(id: String) -> void:
	if StoryData.find(story, "places", id).is_empty():
		id = story["places"][0]["id"] if not story["places"].is_empty() else ""
	place_id = id
	rig.position = Vector3.ZERO
	_rebuild_view()
	_refresh_place_picker()


func _place() -> Dictionary:
	return StoryData.find(story, "places", place_id)


func _rebuild_view() -> void:
	if place_node:
		viewport.remove_child(place_node)
		place_node.queue_free()
	place_node = Builder.build_place(story, place_id, {}, true)
	viewport.add_child(place_node)
	_ring = null
	_highlight()


func _highlight() -> void:
	if is_instance_valid(_ring):
		_ring.queue_free()
	_ring = null
	var key := _selected_key()
	if key == "" or place_node == null:
		return
	var node: Node3D = place_node.get_meta("entities").get(key)
	if node == null:
		return
	_ring = Builder.make_ring(_pick_radius(key), SELECT_COLOR)
	node.add_child(_ring)


func _on_view_input(event: InputEvent) -> void:
	if rig.handle(event):
		view.accept_event()
		return
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT:
		view.grab_focus()
		if event.pressed:
			var point = _ground(event.position)
			if point != null:
				_click(point, event.shift_pressed)
		elif dragging != "":
			dragging = ""
			if _drag_moved:
				_changed(true, false, true)
		view.accept_event()
	elif event is InputEventMouseMotion and dragging != "":
		var point = _ground(event.position)
		if point != null:
			_drag_to(point)
		view.accept_event()
	elif event is InputEventKey and event.pressed and not event.echo:
		match event.keycode:
			KEY_DELETE, KEY_BACKSPACE:
				_delete_selected()
				view.accept_event()
			KEY_R:
				_rotate_selected(-45.0 if event.shift_pressed else 45.0)
				view.accept_event()
			KEY_ESCAPE:
				_set_tool("select")
				view.accept_event()


## Where a point on screen meets the ground, as (x, z), or null.
func _ground(screen: Vector2):
	var camera: Camera3D = rig.camera
	var from := camera.project_ray_origin(screen)
	var direction := camera.project_ray_normal(screen)
	if absf(direction.y) < 0.0001:
		return null
	var t := -from.y / direction.y
	if t < 0.0:
		return null
	var hit := from + direction * t
	return Vector2(hit.x, hit.z)


func _click(point: Vector2, keep_tool: bool) -> void:
	var place := _place()
	if place.is_empty():
		return
	var spot := [snappedf(point.x, GRID), snappedf(point.y, GRID)]
	if tool == "select":
		var hit := _pick(point)
		if hit == "":
			_select("place", place_id)
		else:
			_select_key(hit)
			dragging = hit
			_drag_moved = false
		return
	var key := ""
	match tool:
		"door":
			var to := ""
			for other in story["places"]:
				if other["id"] != place_id:
					to = other["id"]
					break
			place["doors"].append({"pos": spot, "rot": 0.0, "to": to, "label": ""})
			key = "door:%d" % (place["doors"].size() - 1)
		"start":
			story["start_place"] = place_id
			story["start_pos"] = spot
			key = "start"
		"character":
			var character := {"id": StoryData.new_id(story, "characters", "person"), "name": "New person",
					"place": place_id, "pos": spot, "lines": [StoryData.new_line()]}
			StoryData.fill_character(character)
			story["characters"].append(character)
			key = "character:" + character["id"]
		"item":
			var item := {"id": StoryData.new_id(story, "items", "item"), "name": "New item",
					"place": place_id, "pos": spot}
			StoryData.fill_item(item)
			story["items"].append(item)
			key = "item:" + item["id"]
		_:
			if tool.begins_with("prop:"):
				var kind := tool.substr(5)
				place["props"].append({"kind": kind, "pos": spot, "rot": 0.0, "color": StoryData.PROP_COLORS[kind]})
				key = "prop:%d" % (place["props"].size() - 1)
	_changed(true, true)
	_select_key(key)
	if not keep_tool:
		_set_tool("select")


## The thing under a ground point, as an entity key, or "".
func _pick(point: Vector2) -> String:
	var best := ""
	var best_score := 1.0
	for key in place_node.get_meta("entities"):
		var node: Node3D = place_node.get_meta("entities")[key]
		var score := Vector2(node.position.x, node.position.z).distance_to(point) / _pick_radius(key)
		if score < best_score:
			best = key
			best_score = score
	return best


func _pick_radius(key: String) -> float:
	var parts := key.split(":")
	match parts[0]:
		"prop":
			var kind: String = _place()["props"][int(parts[1])]["kind"]
			return StoryData.PROP_RADIUS.get(kind, 0.8)
		"door":
			return 1.0
		"character", "start":
			return 0.7
	return 0.6


func _drag_to(point: Vector2) -> void:
	var spot := [snappedf(point.x, GRID), snappedf(point.y, GRID)]
	if dragging == "start":
		story["start_pos"] = spot
	else:
		var entry := _entry(dragging)
		if entry.is_empty():
			return
		entry["pos"] = spot
	var node: Node3D = place_node.get_meta("entities").get(dragging)
	if node:
		node.position = Vector3(spot[0], node.position.y, spot[1])
	_drag_moved = true
	dirty = true
	_update_title()


## The data behind an entity key.
func _entry(key: String) -> Dictionary:
	var parts := key.split(":")
	match parts[0]:
		"prop", "door":
			var list: Array = _place().get(parts[0] + "s", [])
			var i := int(parts[1])
			return list[i] if i < list.size() else {}
		"character":
			return StoryData.find(story, "characters", parts[1])
		"item":
			return StoryData.find(story, "items", parts[1])
	return {}


func _rotate_selected(degrees: float) -> void:
	var entry := _entry(_selected_key())
	if entry.is_empty() or not entry.has("rot"):
		return
	entry["rot"] = fposmod(entry["rot"] + degrees, 360.0)
	_changed(true, false, true, [entry, "rot"])


func _delete_selected() -> void:
	match selected["kind"]:
		"prop", "door":
			_place()[selected["kind"] + "s"].remove_at(selected["id"])
			_select("place", place_id)
			_changed(true, true)
		"character":
			_delete_character(selected["id"])
		"item":
			_delete_item(selected["id"])


func _set_tool(id: String) -> void:
	tool = id
	tool_buttons[id].button_pressed = true
	var hints := {
		"select": "Click something to select it and drag it to move it. R turns it, Delete removes it. Right-drag turns the camera, the wheel zooms and WASD moves around.",
		"character": "Click the ground to place a new person. Hold Shift to place several.",
		"item": "Click the ground to place an item students can pick up.",
		"door": "Click the ground to place a door. Choose where it leads on the right.",
		"start": "Click the ground where students start the story.",
	}
	hint.text = hints.get(id, "Click the ground to place it. Hold Shift to place several.")


# Selection -------------------------------------------------------------------

func _select(kind: String, id) -> void:
	selected = {"kind": kind, "id": id}
	_highlight()
	_outline_pending = true
	_inspector_pending = true


func _select_key(key: String) -> void:
	var parts := key.split(":")
	match parts[0]:
		"prop", "door":
			_select(parts[0], int(parts[1]))
		"character", "item":
			_select(parts[0], parts[1])
		"start":
			_select("story", "")


func _selected_key() -> String:
	match selected["kind"]:
		"prop", "door":
			return "%s:%d" % [selected["kind"], selected["id"]]
		"character", "item":
			return "%s:%s" % [selected["kind"], selected["id"]]
	return ""


## Shows a character or item, switching to its place first.
func _go_to(kind: String, id: String) -> void:
	var entry := StoryData.find(story, kind + "s", id)
	if entry.is_empty():
		return
	if entry["place"] != place_id and not _place_missing(entry["place"]):
		_show_place(entry["place"])
	rig.position = Vector3(entry["pos"][0], 0.0, entry["pos"][1])
	_select(kind, id)


func _place_missing(id: String) -> bool:
	return StoryData.find(story, "places", id).is_empty()


# Changes ---------------------------------------------------------------------

## Call after every edit. `field` is the [entry, key] being edited, so a
## burst of edits to it (like typing) undoes as one step.
func _changed(view_changed := false, outline_changed := false, inspector_changed := false, field := []) -> void:
	_record(field)
	dirty = true
	_draft_pending = StoryFiles.on_web()
	_update_title()
	if view_changed:
		_rebuild_view()
	if outline_changed:
		_outline_pending = true
	if inspector_changed:
		_inspector_pending = true


func _setter(entry: Dictionary, key: String, view_changed := false, outline_changed := false) -> Callable:
	return func(value):
		entry[key] = value
		_changed(view_changed, outline_changed, false, [entry, key])


# Undo ------------------------------------------------------------------------

## Keeps the story as it was before this edit so it can be undone.
func _record(field: Array) -> void:
	if story == _last:
		return
	var now := Time.get_ticks_msec()
	var same_burst: bool = not field.is_empty() and not _field.is_empty() and is_same(field[0], _field[0]) \
			and field[1] == _field[1] and now - _field_time < BURST_MS
	if not same_burst:
		_undo.append(_last)
		if _undo.size() > HISTORY:
			_undo.pop_front()
	_redo.clear()
	_last = story.duplicate(true)
	_field = field
	_field_time = now
	_update_history_buttons()


func undo() -> void:
	if _undo.is_empty():
		return
	_redo.append(_last)
	_restore(_undo.pop_back())


func redo() -> void:
	if _redo.is_empty():
		return
	_undo.append(_last)
	_restore(_redo.pop_back())


## Puts back an earlier version of the story, staying on the same place and
## selection where they still exist.
func _restore(snapshot: Dictionary) -> void:
	story = snapshot.duplicate(true)
	_last = snapshot
	_field = []
	dragging = ""
	if _place_missing(place_id):
		place_id = story["start_place"]
		rig.position = Vector3.ZERO
	var kind: String = selected["kind"]
	var exists: bool = kind == "story" or (kind == "place" and selected["id"] == place_id) \
			or not _entry(_selected_key()).is_empty()
	if not exists:
		selected = {"kind": "place", "id": place_id}
	dirty = true
	_draft_pending = StoryFiles.on_web()
	_update_title()
	_rebuild_view()
	_refresh_place_picker()
	_outline_pending = true
	_inspector_pending = true
	_update_history_buttons()


func _update_history_buttons() -> void:
	undo_button.disabled = _undo.is_empty()
	redo_button.disabled = _redo.is_empty()


func _update_title() -> void:
	title_label.text = story["title"] + ("  *" if dirty else "")
	if status_label.text == "" or status_label.text.begins_with("File: ") or status_label.text == "Not saved yet":
		status_label.text = ("File: " + path.get_file()) if path != "" else "Not saved yet"


func _status(text: String) -> void:
	status_label.text = text


func _add_place() -> void:
	var place := {"id": StoryData.new_id(story, "places", "place"), "name": "New place"}
	StoryData.fill_place(place)
	story["places"].append(place)
	_changed(false, true)
	_show_place(place["id"])
	_select("place", place["id"])


func _delete_place(id: String) -> void:
	if story["places"].size() <= 1:
		return
	_ask("Delete this place? People and items in it will have no place until you move them.", func():
		story["places"].erase(StoryData.find(story, "places", id))
		for place in story["places"]:
			for door in place["doors"]:
				if door["to"] == id:
					door["to"] = ""
		if story["start_place"] == id:
			story["start_place"] = story["places"][0]["id"]
		_changed(false, true)
		_show_place(story["places"][0]["id"])
		_select("place", place_id)
	)


func _add_character() -> void:
	_set_tool("character")
	_click(Vector2(rig.position.x, rig.position.z), false)


func _delete_character(id: String) -> void:
	var character := StoryData.find(story, "characters", id)
	_ask("Delete %s and everything they say?" % character["name"], func():
		story["characters"].erase(character)
		_select("place", place_id)
		_changed(true, true)
	)


func _add_item() -> void:
	_set_tool("item")
	_click(Vector2(rig.position.x, rig.position.z), false)


func _delete_item(id: String) -> void:
	story["items"].erase(StoryData.find(story, "items", id))
	_select("place", place_id)
	_changed(true, true)


# Files -----------------------------------------------------------------------

func _on_new() -> void:
	_ask_if_dirty(func(): load_story(StoryData.blank(), ""))


func _on_open() -> void:
	_ask_if_dirty(files.ask_open)


func _on_save() -> void:
	if StoryFiles.on_web() or path == "" or path.begins_with("res://"):
		files.ask_save(story)
	else:
		files.save(story, path)


func _on_save_as() -> void:
	files.ask_save(story)


func _on_saved(file_path: String) -> void:
	path = file_path
	dirty = false
	_status(("Downloaded " if StoryFiles.on_web() else "Saved ") + file_path.get_file())
	_update_title()


## Copies a link that plays the story as it is now. The whole story is in the
## link, so nothing has to be uploaded anywhere.
func _on_copy_link() -> void:
	DisplayServer.clipboard_set(StoryFiles.link_for(story))
	var problems := StoryData.problems(story).size()
	if problems > 0:
		_status("Link copied, but the story still has %d problem%s to fix." % [problems, "" if problems == 1 else "s"])
	else:
		_status("Link copied. Students open it to play this version of the story.")


## Keeps the story in the browser so the menu can offer to continue it.
func _save_draft() -> void:
	_draft_timer = 0.0
	if _draft_pending:
		_draft_pending = false
		StoryFiles.save_draft(story)


func _on_play() -> void:
	_save_draft()
	play_requested.emit(story.duplicate(true))


func _on_exit() -> void:
	_save_draft()
	_ask_if_dirty(func(): exit_requested.emit())


func _ask_if_dirty(then: Callable) -> void:
	if dirty:
		_ask("Your story has unsaved changes. Continue without saving them?", then)
	else:
		then.call()


func _ask(text: String, then: Callable) -> void:
	confirm.dialog_text = text
	_on_confirm = then
	confirm.popup_centered()


# Outline (left) --------------------------------------------------------------

func _rebuild_outline() -> void:
	_clear(outline)
	_section(outline, "STORY")
	outline.add_child(_list_button(story["title"], selected["kind"] == "story", func(): _select("story", "")))

	_section(outline, "PLACES")
	for place in story["places"]:
		var id: String = place["id"]
		var label: String = place["name"] + ("  (indoors)" if place["indoor"] else "")
		outline.add_child(_list_button(label, selected["kind"] == "place" and selected["id"] == id, func():
			_show_place(id)
			_select("place", id)
		))
	outline.add_child(_list_button("+ Add place", false, _add_place))

	_section(outline, "PEOPLE")
	for character in story["characters"]:
		var id: String = character["id"]
		var button := _list_button(character["name"], selected["kind"] == "character" and selected["id"] == id, _go_to.bind("character", id))
		button.add_theme_color_override("font_color", Color(character["color"]).darkened(0.35))
		outline.add_child(button)
	outline.add_child(_list_button("+ Add person", false, _add_character))

	_section(outline, "ITEMS")
	for item in story["items"]:
		var id: String = item["id"]
		outline.add_child(_list_button(item["name"], selected["kind"] == "item" and selected["id"] == id, _go_to.bind("item", id)))
	outline.add_child(_list_button("+ Add item", false, _add_item))

	_section(outline, "CHECK")
	var problems := StoryData.problems(story)
	if problems.is_empty():
		outline.add_child(_note("Everything looks ready to play.", "GoodLabel"))
	for problem in problems:
		outline.add_child(_note(problem, "WarningLabel"))


func _list_button(text: String, active: bool, on_press: Callable) -> Button:
	var button := Button.new()
	button.text = text
	button.theme_type_variation = "ListButton"
	button.alignment = HORIZONTAL_ALIGNMENT_LEFT
	button.toggle_mode = true
	button.button_pressed = active
	button.focus_mode = Control.FOCUS_NONE
	button.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	button.pressed.connect(on_press)
	return button


func _refresh_place_picker() -> void:
	place_picker.clear()
	for i in story["places"].size():
		place_picker.add_item(story["places"][i]["name"], i)
		if story["places"][i]["id"] == place_id:
			place_picker.select(i)


# Inspector (right) -----------------------------------------------------------

func _rebuild_inspector() -> void:
	_clear(inspector)
	match selected["kind"]:
		"story":
			_inspect_story()
		"place":
			_inspect_place()
		"prop":
			_inspect_prop()
		"door":
			_inspect_door()
		"character":
			_inspect_character()
		"item":
			_inspect_item()


func _inspect_story() -> void:
	_header("Story")
	_row("Title", _line_edit(story["title"], func(v):
		story["title"] = v
		_changed(false, true, false, [story, "title"])
	))
	_row("Introduction, shown when the game starts", _text_edit(story["intro"], _setter(story, "intro")))
	_row("Goal, shown while playing", _line_edit(story["goal"], _setter(story, "goal")))
	_row("Starting place", _option(_place_options(), story["start_place"], _setter(story, "start_place", true)))
	inspector.add_child(_note("Use the Start point tool to choose where students begin.", "MutedLabel"))


func _inspect_place() -> void:
	var place := _place()
	if place.is_empty():
		return
	_header("Place")
	_row("Name", _line_edit(place["name"], func(v):
		place["name"] = v
		_changed(true, true, false, [place, "name"])
		_refresh_place_picker()
	))
	inspector.add_child(_check("Indoors (walls and lamp light)", place["indoor"], func(v):
		place["indoor"] = v
		_changed(true, true, true, [place, "indoor"])
	))
	_row("Size in metres", _spin(place["size"], 8, 120, 2, _setter(place, "size", true)))
	_row("Ground or floor colour", _color(place["ground"], _setter(place, "ground", true)))
	_row("Sky or wall colour", _color(place["sky"], _setter(place, "sky", true)))
	if not place["indoor"]:
		var weathers := [["clear", "Clear, a few clouds"], ["cloudy", "Cloudy"], ["rain", "Rain"]]
		_row("Weather", _option(weathers, place["weather"], _setter(place, "weather", true)))
	var delete := _danger_button("Delete place", _delete_place.bind(place_id))
	delete.disabled = story["places"].size() <= 1
	inspector.add_child(delete)


func _inspect_prop() -> void:
	var props: Array = _place().get("props", [])
	var index: int = selected["id"]
	if index >= props.size():
		return
	var prop: Dictionary = props[index]
	_header(String(prop["kind"]).capitalize())
	var kinds := []
	for kind in StoryData.PROP_KINDS:
		kinds.append([kind, kind.capitalize()])
	_row("Type", _option(kinds, prop["kind"], func(v):
		prop["kind"] = v
		prop["color"] = StoryData.PROP_COLORS[v]
		_changed(true, false, true)
	))
	_row("Colour", _color(prop["color"], _setter(prop, "color", true)))
	_row("Turn (degrees)", _spin(prop["rot"], 0, 345, 15, _setter(prop, "rot", true)))
	inspector.add_child(_danger_button("Delete", _delete_selected))


func _inspect_door() -> void:
	var doors: Array = _place().get("doors", [])
	var index: int = selected["id"]
	if index >= doors.size():
		return
	var door: Dictionary = doors[index]
	_header("Door")
	var targets := [["", "Nowhere yet"]]
	for place in story["places"]:
		if place["id"] != place_id:
			targets.append([place["id"], place["name"]])
	_row("Leads to", _option(targets, door["to"], _setter(door, "to", true, true)))
	_row("Sign (leave empty to show the place's name)", _line_edit(door["label"], _setter(door, "label", true)))
	_row("Turn (degrees)", _spin(door["rot"], 0, 345, 15, _setter(door, "rot", true)))
	inspector.add_child(_note("Students coming the other way appear in front of the door that leads back here, so give each place a door back.", "MutedLabel"))
	inspector.add_child(_danger_button("Delete", _delete_selected))


func _inspect_item() -> void:
	var item := StoryData.find(story, "items", selected["id"])
	if item.is_empty():
		return
	_header("Item")
	_row("Name", _line_edit(item["name"], func(v):
		item["name"] = v
		_changed(true, true, false, [item, "name"])
	))
	_row("What students read when they pick it up", _text_edit(item["description"], _setter(item, "description")))
	_row("Colour", _color(item["color"], _setter(item, "color", true)))
	_row("Place", _option([["", "Nowhere (only given in conversations)"]] + _place_options(), item["place"], func(v):
		item["place"] = v
		_changed(true, true)
	))
	inspector.add_child(_danger_button("Delete item", _delete_item.bind(item["id"])))


func _inspect_character() -> void:
	var character := StoryData.find(story, "characters", selected["id"])
	if character.is_empty():
		return
	_header("Character")
	_row("Name", _line_edit(character["name"], func(v):
		character["name"] = v
		_changed(true, true, false, [character, "name"])
	))
	var looks := [["person", "A person"]]
	for kind in StoryData.ANIMALS:
		looks.append([kind, "A talking " + kind])
	_row("Looks like", _option(looks, character["look"], func(v):
		character["look"] = v
		# A new animal starts in its own colours.
		if v != "person":
			character["color"] = StoryData.PROP_COLORS[v]
		_changed(true, true, true, [character, "look"])
	))
	_row("Colour", _color(character["color"], _setter(character, "color", true, true)))
	_row("Place", _option(_place_options(), character["place"], func(v):
		character["place"] = v
		_changed(true, true)
	))
	_row("Turn (degrees)", _spin(character["rot"], 0, 345, 15, _setter(character, "rot", true)))

	_header("What they say")
	inspector.add_child(_note("A conversation starts at the first line that can start one and whose conditions hold. Each choice can lead to another line, give or take an item, set a fact for later, or end the story.", "MutedLabel"))
	var lines: Array = character["lines"]
	for i in lines.size():
		inspector.add_child(_line_panel(character, i))
	inspector.add_child(_button("+ Add line", func():
		lines.append(StoryData.new_line("..."))
		lines[-1]["opens"] = false
		_changed(false, true, true)
	))
	inspector.add_child(_danger_button("Delete person", _delete_character.bind(character["id"])))


func _line_panel(character: Dictionary, index: int) -> Control:
	var lines: Array = character["lines"]
	var line: Dictionary = lines[index]
	var panel := PanelContainer.new()
	panel.theme_type_variation = "LinePanel"
	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 8)
	panel.add_child(box)

	var head := HBoxContainer.new()
	box.add_child(head)
	var title := Label.new()
	title.text = "Line %d" % (index + 1)
	title.theme_type_variation = "HeaderLabel"
	title.add_theme_font_size_override("font_size", 18)
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	head.add_child(title)
	head.add_child(_small_button("Delete line", func(): _delete_line(character, index)))

	box.add_child(_check("Can start a conversation", line["opens"], func(v):
		line["opens"] = v
		_changed(false, true, true)
	))
	if line["opens"]:
		var when := GridContainer.new()
		when.columns = 2
		box.add_child(when)
		_grid_row(when, "Only if they have", _option(_fact_options(), line["requires"], _setter(line, "requires", false, true)))
		_grid_row(when, "Not if they have", _option(_fact_options(), line["unless"], _setter(line, "unless", false, true)))
	box.add_child(_text_edit(line["text"], _setter(line, "text")))

	var choices: Array = line["choices"]
	for c in choices.size():
		box.add_child(_choice_panel(lines, choices, c))
	box.add_child(_small_button("+ Add choice", func():
		choices.append(StoryData.new_choice("..."))
		_changed(false, true, true)
	))
	return panel


func _choice_panel(lines: Array, choices: Array, index: int) -> Control:
	var choice: Dictionary = choices[index]
	var panel := PanelContainer.new()
	panel.theme_type_variation = "ChoicePanel"
	var box := VBoxContainer.new()
	panel.add_child(box)
	var row := HBoxContainer.new()
	box.add_child(row)
	var text := _line_edit(choice["text"], _setter(choice, "text"), "What the student says")
	text.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row.add_child(text)
	row.add_child(_small_button("x", func():
		choices.remove_at(index)
		_changed(false, true, true)
	))

	var next_options := [[-1, "Then: stop talking"]]
	for i in lines.size():
		next_options.append([i, "Then: line %d" % (i + 1)])
	box.add_child(_option(next_options, choice["next"], _setter(choice, "next", false, true)))

	var details := GridContainer.new()
	details.columns = 2
	_grid_row(details, "Only if they have", _option(_fact_options(), choice["requires"], _setter(choice, "requires", false, true)))
	_grid_row(details, "Not if they have", _option(_fact_options(), choice["unless"], _setter(choice, "unless", false, true)))
	_grid_row(details, "Gives", _option(_item_options(), choice["gives"], _setter(choice, "gives", false, true)))
	_grid_row(details, "Takes", _option(_item_options(), choice["takes"], _setter(choice, "takes", false, true)))
	_grid_row(details, "Sets the fact", _line_edit(choice["sets"], _setter(choice, "sets", false, true), "e.g. met_mayor"))
	_grid_row(details, "Ends the story", _line_edit(choice["ending"], _setter(choice, "ending", false, true), "Ending text"))
	var used := false
	for key in StoryData.CHOICE_FIELDS:
		used = used or choice[key] != ""
	details.visible = used
	var toggle := _small_button("Conditions and effects", func(): details.visible = not details.visible)
	toggle.size_flags_horizontal = Control.SIZE_SHRINK_BEGIN
	box.add_child(toggle)
	box.add_child(details)
	return panel


func _delete_line(character: Dictionary, index: int) -> void:
	var lines: Array = character["lines"]
	lines.remove_at(index)
	for line in lines:
		for choice in line["choices"]:
			if choice["next"] == index:
				choice["next"] = -1
			elif choice["next"] > index:
				choice["next"] -= 1
	_changed(false, true, true)


func _place_options() -> Array:
	var out := []
	for place in story["places"]:
		out.append([place["id"], place["name"]])
	return out


func _item_options() -> Array:
	var out := [["", "(nothing)"]]
	for item in story["items"]:
		out.append([item["id"], item["name"]])
	return out


func _fact_options() -> Array:
	var out := _item_options()
	for flag in StoryData.flags(story):
		out.append([flag, "the fact \"%s\"" % flag])
	return out


# Widgets ---------------------------------------------------------------------

func _clear(container: Control) -> void:
	for child in container.get_children():
		container.remove_child(child)
		child.queue_free()


func _section(container: Control, text: String) -> void:
	var label := Label.new()
	label.text = text
	label.theme_type_variation = "SectionLabel"
	container.add_child(label)


func _header(text: String) -> void:
	var label := Label.new()
	label.text = text
	label.theme_type_variation = "HeaderLabel"
	inspector.add_child(label)


func _note(text: String, variation: String) -> Label:
	var label := Label.new()
	label.text = text
	label.theme_type_variation = variation
	label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	label.custom_minimum_size.x = 200
	return label


func _row(label_text: String, field: Control) -> void:
	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 4)
	var label := Label.new()
	label.text = label_text
	label.theme_type_variation = "FieldLabel"
	label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	box.add_child(label)
	field.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	box.add_child(field)
	inspector.add_child(box)


func _grid_row(grid: GridContainer, label_text: String, field: Control) -> void:
	var label := Label.new()
	label.text = label_text
	label.theme_type_variation = "FieldLabel"
	grid.add_child(label)
	field.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	grid.add_child(field)


func _button(text: String, on_press: Callable) -> Button:
	var button := Button.new()
	button.text = text
	button.focus_mode = Control.FOCUS_NONE
	button.pressed.connect(on_press)
	return button


func _small_button(text: String, on_press: Callable) -> Button:
	var button := _button(text, on_press)
	button.theme_type_variation = "SmallButton"
	return button


func _danger_button(text: String, on_press: Callable) -> Button:
	var button := _button(text, on_press)
	button.add_theme_color_override("font_color", Color("#b5462f"))
	button.size_flags_horizontal = Control.SIZE_SHRINK_BEGIN
	return button


func _line_edit(value: String, on_change: Callable, placeholder := "") -> LineEdit:
	var edit := LineEdit.new()
	edit.text = value
	edit.placeholder_text = placeholder
	edit.text_changed.connect(on_change)
	return edit


func _text_edit(value: String, on_change: Callable) -> TextEdit:
	var edit := TextEdit.new()
	edit.text = value
	edit.wrap_mode = TextEdit.LINE_WRAPPING_BOUNDARY
	edit.custom_minimum_size.y = 80
	edit.scroll_fit_content_height = true
	edit.text_changed.connect(func(): on_change.call(edit.text))
	return edit


## A dropdown of [value, text] pairs that calls `on_change` with the value.
func _option(items: Array, current, on_change: Callable) -> OptionButton:
	var option := OptionButton.new()
	option.fit_to_longest_item = false
	option.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	for i in items.size():
		option.add_item(str(items[i][1]), i)
		if typeof(items[i][0]) == typeof(current) and items[i][0] == current:
			option.select(i)
	option.item_selected.connect(func(i): on_change.call(items[i][0]))
	return option


func _check(text: String, value: bool, on_change: Callable) -> CheckBox:
	var check := CheckBox.new()
	check.text = text
	check.button_pressed = value
	check.toggled.connect(on_change)
	return check


func _spin(value: float, low: float, high: float, step: float, on_change: Callable) -> SpinBox:
	var spin := SpinBox.new()
	spin.min_value = low
	spin.max_value = high
	spin.step = step
	spin.value = value
	spin.value_changed.connect(on_change)
	return spin


func _color(value: String, on_change: Callable) -> ColorPickerButton:
	var picker := ColorPickerButton.new()
	picker.color = Color(value)
	picker.edit_alpha = false
	picker.custom_minimum_size.y = 34
	picker.color_changed.connect(func(c: Color): on_change.call("#" + c.to_html(false)))
	return picker
