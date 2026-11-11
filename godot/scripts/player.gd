extends CharacterBody3D
## The student's character: walks relative to the camera and animates.

const Builder = preload("res://scripts/world_builder.gd")

const SPEED := 4.5
const GRAVITY := 20.0

## Set by play mode while a conversation or transition is running.
var frozen := false
## The camera rig, whose yaw decides which way "forward" is.
var rig
## The animated person (actor.gd).
var visual


func _init() -> void:
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


func _physics_process(delta: float) -> void:
	var input := Vector2.ZERO
	if not frozen:
		input = Input.get_vector("move_left", "move_right", "move_forward", "move_back")
	var yaw: float = rig.yaw if rig else 0.0
	var direction := Vector3(input.x, 0, input.y).rotated(Vector3.UP, yaw)
	velocity.x = direction.x * SPEED
	velocity.z = direction.z * SPEED
	velocity.y = 0.0 if is_on_floor() else velocity.y - GRAVITY * delta
	move_and_slide()
	visual.moving = direction.length() > 0.1
	if visual.moving:
		visual.face_yaw(atan2(direction.x, direction.z))
