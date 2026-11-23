extends Control
## An on-screen joystick for touch screens. Holds the move actions down as far
## as the knob is pushed, so the student walks just like with the keys.

const RADIUS := 80.0
const SIZE := 220.0

## The finger on the stick, or -1.
var _finger := -1
var _knob := Vector2.ZERO


func _init() -> void:
	custom_minimum_size = Vector2(SIZE, SIZE)
	size = custom_minimum_size


func _gui_input(event: InputEvent) -> void:
	if event is InputEventScreenTouch:
		if event.pressed and _finger < 0:
			_finger = event.index
			_push(event.position)
		elif not event.pressed and event.index == _finger:
			_let_go()
		accept_event()
	elif event is InputEventScreenDrag:
		if event.index == _finger:
			_push(event.position)
		accept_event()


func _notification(what: int) -> void:
	if (what == NOTIFICATION_VISIBILITY_CHANGED and not is_visible_in_tree()) or what == NOTIFICATION_EXIT_TREE:
		_let_go()


func _push(at: Vector2) -> void:
	_knob = (at - size / 2).limit_length(RADIUS)
	var v := _knob / RADIUS
	_hold("move_left", -v.x)
	_hold("move_right", v.x)
	_hold("move_forward", -v.y)
	_hold("move_back", v.y)
	queue_redraw()


func _let_go() -> void:
	if _finger < 0:
		return
	_finger = -1
	_knob = Vector2.ZERO
	for action in ["move_left", "move_right", "move_forward", "move_back"]:
		Input.action_release(action)
	queue_redraw()


func _hold(action: String, strength: float) -> void:
	if strength > 0.0:
		Input.action_press(action, strength)
	else:
		Input.action_release(action)


func _draw() -> void:
	var center := size / 2
	draw_circle(center, RADIUS + 10.0, Color(1, 1, 1, 0.25))
	draw_arc(center, RADIUS + 10.0, 0.0, TAU, 64, Color(0.2, 0.16, 0.1, 0.35), 3.0, true)
	draw_circle(center + _knob, 38.0, Color(1, 1, 1, 0.85 if _finger >= 0 else 0.6))
	draw_arc(center + _knob, 38.0, 0.0, TAU, 48, Color(0.2, 0.16, 0.1, 0.4), 2.0, true)
