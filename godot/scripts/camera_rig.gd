extends Node3D
## A third-person orbit camera: follows a target smoothly, orbits with the
## right (or middle) mouse button or one finger, zooms with the wheel or a
## pinch, and pulls in instead of clipping through walls.

var target: Node3D
var yaw := 0.0
var pitch := deg_to_rad(-38.0)
var distance := 10.0
var min_distance := 4.0
var max_distance := 40.0
## Radians per second, for slowly circling a scene (the menu background).
var auto_orbit := 0.0
## Moves the focus point with the movement keys (the editor's free camera).
var pan_keys := false
## Reads mouse and touch input by itself. The editor turns this off and calls handle().
var listen := true

var arm: SpringArm3D
var camera: Camera3D
var _orbiting := false
## Fingers on the 3D view (index -> position). Fingers that start on the HUD
## never get here, because the HUD takes their touches first.
var _touches := {}


func _init() -> void:
	arm = SpringArm3D.new()
	arm.spring_length = distance
	arm.margin = 0.3
	arm.collision_mask = 1
	add_child(arm)
	camera = Camera3D.new()
	camera.fov = 50.0
	camera.current = true
	arm.add_child(camera)


func exclude(body: CollisionObject3D) -> void:
	arm.add_excluded_object(body.get_rid())


## Jumps straight to the target instead of gliding there.
func snap() -> void:
	if target and target.is_inside_tree():
		global_position = _focus()
	arm.spring_length = distance


func _focus() -> Vector3:
	return target.global_position + Vector3(0, 1.1, 0)


func _process(delta: float) -> void:
	if target and target.is_inside_tree():
		global_position = global_position.lerp(_focus(), 1.0 - exp(-delta * 7.0))
	yaw += auto_orbit * delta
	if pan_keys:
		var move := Input.get_vector("move_left", "move_right", "move_forward", "move_back")
		position += Vector3(move.x, 0, move.y).rotated(Vector3.UP, yaw) * delta * distance * 0.9
	arm.spring_length = lerpf(arm.spring_length, distance, 1.0 - exp(-delta * 10.0))
	rotation = Vector3(pitch, yaw, 0)


## Handles orbiting and zooming. Returns whether the event was used.
func handle(event: InputEvent) -> bool:
	if event is InputEventMouseButton:
		match event.button_index:
			MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MIDDLE:
				_orbiting = event.pressed
				return true
			MOUSE_BUTTON_WHEEL_UP:
				if event.pressed:
					distance = maxf(min_distance, distance * 0.9)
				return true
			MOUSE_BUTTON_WHEEL_DOWN:
				if event.pressed:
					distance = minf(max_distance, distance * 1.1)
				return true
	elif event is InputEventMouseMotion and _orbiting:
		_orbit(event.relative)
		return true
	return false


## Handles touch: one finger orbits, two fingers pinch to zoom. Mouse events
## that Godot makes up from touches use the left button, so they never orbit.
## Returns whether the event was used.
func touch(event: InputEvent) -> bool:
	if event is InputEventScreenTouch:
		if event.pressed:
			_touches[event.index] = event.position
			return true
		return _touches.erase(event.index)
	if event is InputEventScreenDrag and _touches.has(event.index):
		if _touches.size() == 1:
			_orbit(event.relative)
		else:
			var other: Vector2 = _touches[_touches.keys().filter(func(i): return i != event.index)[0]]
			var before: float = _touches[event.index].distance_to(other)
			var after: float = event.position.distance_to(other)
			if before > 1.0 and after > 1.0:
				distance = clampf(distance * before / after, min_distance, max_distance)
		_touches[event.index] = event.position
		return true
	return false


func _orbit(relative: Vector2) -> void:
	yaw -= relative.x * 0.008
	pitch = clampf(pitch - relative.y * 0.006, deg_to_rad(-80.0), deg_to_rad(-8.0))


func _unhandled_input(event: InputEvent) -> void:
	if listen and (handle(event) or touch(event)):
		get_viewport().set_input_as_handled()
