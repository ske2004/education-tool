extends Control
## A round "Jump" button for touch screens. Only real touches press it, not
## the mouse clicks Godot makes up from them, so one tap is one jump.

signal pressed

const SIZE := 130.0

var _down := false


func _init() -> void:
	custom_minimum_size = Vector2(SIZE, SIZE)
	size = custom_minimum_size


func _gui_input(event: InputEvent) -> void:
	if event is InputEventScreenTouch:
		_down = event.pressed
		if event.pressed:
			pressed.emit()
		queue_redraw()
		accept_event()
	elif event is InputEventScreenDrag:
		accept_event()


func _notification(what: int) -> void:
	if what == NOTIFICATION_VISIBILITY_CHANGED and not is_visible_in_tree():
		_down = false


func _has_point(point: Vector2) -> bool:
	return point.distance_to(size / 2) <= SIZE / 2


func _draw() -> void:
	var center := size / 2
	var radius := SIZE / 2 - 4.0
	draw_circle(center, radius, Color(1, 1, 1, 0.85 if _down else 0.6))
	draw_arc(center, radius, 0.0, TAU, 64, Color(0.2, 0.16, 0.1, 0.4), 3.0, true)
	var font := get_theme_default_font()
	var font_size := 26
	var text := "Jump"
	var width := font.get_string_size(text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x
	var baseline := center.y + (font.get_ascent(font_size) - font.get_descent(font_size)) / 2
	draw_string(font, Vector2(center.x - width / 2, baseline), text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(0.2, 0.16, 0.1, 0.9))
