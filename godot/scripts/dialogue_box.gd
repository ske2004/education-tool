extends PanelContainer
## Shows what a character says, typed out letter by letter, and the
## student's choices. Emits `chosen` with the picked choice's index.

signal chosen(index: int)

var speaker: Label
var body: RichTextLabel
var options: VBoxContainer
var _choices: Array = []
var _tween: Tween


func _init() -> void:
	theme_type_variation = "Card"
	custom_minimum_size = Vector2(760, 0)
	visible = false
	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 12)
	add_child(box)
	speaker = Label.new()
	speaker.theme_type_variation = "HeaderLabel"
	box.add_child(speaker)
	body = RichTextLabel.new()
	body.fit_content = true
	body.scroll_active = false
	body.custom_minimum_size = Vector2(0, 56)
	body.add_theme_font_size_override("normal_font_size", 20)
	box.add_child(body)
	options = VBoxContainer.new()
	options.add_theme_constant_override("separation", 6)
	box.add_child(options)


## `choices` is [{index, text}]; a choice with index -1 ends the conversation.
func show_line(who: String, color: Color, text: String, choices: Array) -> void:
	visible = true
	speaker.text = who
	speaker.add_theme_color_override("font_color", color.darkened(0.3))
	body.text = text
	body.visible_ratio = 0.0
	_choices = choices
	_clear_options()
	if _tween:
		_tween.kill()
	_tween = create_tween()
	_tween.tween_property(body, "visible_ratio", 1.0, clampf(text.length() * 0.018, 0.2, 2.5))
	_tween.finished.connect(_show_choices)


func typing() -> bool:
	return _tween != null and _tween.is_running()


## Finishes the typing, or picks the only choice when there's just one.
func advance() -> void:
	if typing():
		_finish_typing()
	elif _choices.size() == 1:
		chosen.emit(_choices[0]["index"])


func pick(n: int) -> void:
	if typing():
		_finish_typing()
	elif n >= 0 and n < _choices.size():
		chosen.emit(_choices[n]["index"])


func close() -> void:
	visible = false
	if _tween:
		_tween.kill()
	_clear_options()


func _finish_typing() -> void:
	_tween.kill()
	body.visible_ratio = 1.0
	_show_choices()


func _show_choices() -> void:
	_clear_options()
	for n in _choices.size():
		var button := Button.new()
		button.text = "%d.  %s" % [n + 1, _choices[n]["text"]]
		button.alignment = HORIZONTAL_ALIGNMENT_LEFT
		button.focus_mode = Control.FOCUS_NONE
		button.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		var index: int = _choices[n]["index"]
		button.pressed.connect(func(): chosen.emit(index))
		options.add_child(button)


func _clear_options() -> void:
	for child in options.get_children():
		options.remove_child(child)
		child.queue_free()
