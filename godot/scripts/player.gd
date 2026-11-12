extends CharacterBody3D
## The student's character: walks relative to the camera, jumps, sits on
## benches and animates.

const Builder = preload("res://scripts/world_builder.gd")

## Emitted when the student sits down or stands up.
signal seat_changed
## Emitted when a jump lands in water and the student is put back on the bank.
signal splashed

const SPEED := 4.5
const GRAVITY := 20.0
## About a metre high: sqrt(2 * GRAVITY * 1.0), plus a little.
const JUMP_SPEED := 6.5
## Falling pulls harder than rising, so jumps feel snappy instead of floaty.
const FALL_GRAVITY := 1.6
## A jump pressed this long before landing still happens.
const JUMP_BUFFER := 0.12
## Where students sit on a bench, from its origin: on the seat, against the backrest.
const SEAT := Vector3(0, 0.55, 0.14)
## Where they stand up, in front of the bench.
const IN_FRONT := 1.0

## Set by play mode while a conversation or transition is running.
var frozen := false
## The camera rig, whose yaw decides which way "forward" is.
var rig
## The animated person (actor.gd).
var visual
## The bench being sat on, or null.
var seat: Node3D
var _jump := 0.0
## Where the student last stood on dry ground, to go back to after jumping into water.
var _dry := Vector3.ZERO


func _init() -> void:
	## Animals find the student through this group.
	add_to_group("student")
	collision_mask = 1 | Builder.WALL_LAYER
	var capsule := CapsuleShape3D.new()
	capsule.radius = 0.35
	capsule.height = 1.7
	var shape := CollisionShape3D.new()
	shape.shape = capsule
	shape.position.y = 0.85
	add_child(shape)
	visual = Builder.make_person(Color("#4a7fd4"))
	add_child(visual)


## Jumps from the floor, or stands up when sitting.
func jump() -> void:
	if frozen:
		return
	if seat:
		stand_up()
	else:
		_jump = JUMP_BUFFER


## Sits on a bench, facing the way it faces (its local +z).
func sit_on(bench: Node3D) -> void:
	seat = bench
	_jump = 0.0
	velocity = Vector3.ZERO
	global_position = bench.global_transform * SEAT
	var forward := bench.global_basis.z
	visual.face_yaw(atan2(forward.x, forward.z))
	visual.sitting = true
	visual.airborne = false
	visual.moving = false
	seat_changed.emit()


## Stands up in front of the bench. Does nothing when not sitting.
func stand_up() -> void:
	if seat == null:
		return
	if seat.is_inside_tree():
		var forward := seat.global_basis.z
		global_position = seat.global_position + Vector3(forward.x, 0, forward.z).normalized() * IN_FRONT + Vector3(0, 0.05, 0)
	seat = null
	visual.sitting = false
	seat_changed.emit()


func _physics_process(delta: float) -> void:
	var input := Vector2.ZERO
	if not frozen:
		input = Input.get_vector("move_left", "move_right", "move_forward", "move_back")
	if seat:
		if input.length() > 0.1:
			stand_up()
		else:
			return
	var yaw: float = rig.yaw if rig else 0.0
	var direction := Vector3(input.x, 0, input.y).rotated(Vector3.UP, yaw)
	velocity.x = direction.x * SPEED
	velocity.z = direction.z * SPEED
	if frozen:
		_jump = 0.0
	if is_on_floor():
		velocity.y = JUMP_SPEED if _jump > 0.0 else 0.0
		if _jump > 0.0:
			_jump = 0.0
	else:
		velocity.y -= GRAVITY * (FALL_GRAVITY if velocity.y < 0.0 else 1.0) * delta
	_jump = maxf(0.0, _jump - delta)
	move_and_slide()
	if is_on_floor():
		if _on_water():
			global_position = _dry
			velocity = Vector3.ZERO
			splashed.emit()
		else:
			_dry = global_position
	visual.airborne = not is_on_floor()
	visual.moving = direction.length() > 0.1
	if visual.moving:
		visual.face_yaw(atan2(direction.x, direction.z))


## Whether the student is standing on something play mode marked as water.
func _on_water() -> bool:
	for i in get_slide_collision_count():
		var hit := get_slide_collision(i)
		var body := hit.get_collider()
		if hit.get_normal().y > 0.7 and body and body.has_meta("water"):
			return true
	return false
