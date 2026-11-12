extends Node3D
## Animates a person built by WorldBuilder.make_person: breathing while idle,
## bouncing while walking, nodding while talking, tucking in while jumping,
## sitting, and turning smoothly.

## How far the body drops when sitting, so it rests on the seat.
const SIT_DROP := -0.15

var moving := false
var talking := false
## In the air: the body tucks in, and squashes a little on landing.
var airborne := false
## Sitting: the body drops onto the seat and legs point forward.
var sitting := false:
	set(value):
		sitting = value
		if value and _legs == null:
			_add_legs()
		if _legs:
			_legs.visible = value
		if _body:
			_body.position.y = SIT_DROP if value else 0.0
var _t := randf() * TAU
var _yaw_target := NAN
var _body: Node3D
var _legs: Node3D
var _was_airborne := false
var _squash := 0.0


func _ready() -> void:
	_body = get_node_or_null("Body")


func face(point: Vector3) -> void:
	var d := point - global_position
	if Vector2(d.x, d.z).length() > 0.01:
		_yaw_target = atan2(d.x, d.z)


func face_yaw(yaw: float) -> void:
	_yaw_target = yaw


func _process(delta: float) -> void:
	var walking := moving and not airborne and not sitting
	_t += delta * (10.0 if walking else 2.2)
	if _was_airborne and not airborne:
		_squash = 1.0
	_was_airborne = airborne
	_squash = move_toward(_squash, 0.0, delta * 6.0)
	if _body:
		var settle := minf(1.0, delta * 10.0)
		if walking:
			_body.position.y = absf(sin(_t)) * 0.12
			_body.rotation.z = sin(_t) * 0.07
		else:
			var y := sin(_t) * 0.015
			if talking:
				y += absf(sin(_t * 3.0)) * 0.035
			if sitting:
				y += SIT_DROP
			elif airborne:
				y = 0.12
			_body.position.y = lerpf(_body.position.y, y, settle)
			_body.rotation.z = lerpf(_body.rotation.z, 0.0, settle)
		_body.rotation.x = lerpf(_body.rotation.x, 0.18 if airborne else 0.0, settle)
		var tall := 0.9 if airborne else 1.0 - _squash * 0.18
		var wide := 1.0 + (1.0 - tall) * 0.6
		_body.scale = _body.scale.lerp(Vector3(wide, tall, wide), minf(1.0, delta * 20.0))
	if not is_nan(_yaw_target):
		rotation.y = lerp_angle(rotation.y, _yaw_target, minf(1.0, delta * 9.0))


## Thighs along the seat and shins hanging down, in the body's colour.
func _add_legs() -> void:
	_legs = Node3D.new()
	_legs.name = "Legs"
	add_child(_legs)
	var torso: MeshInstance3D = _body.get_child(0) if _body else null
	var cloth: Material = torso.material_override if torso else null
	var dark: StandardMaterial3D = cloth.duplicate() if cloth is StandardMaterial3D else null
	if dark:
		dark.albedo_color = dark.albedo_color.darkened(0.3)
	for x in [-0.14, 0.14]:
		var thigh := _capsule(0.12, 0.55, cloth)
		thigh.position = Vector3(x, 0.1, 0.28)
		thigh.rotation.x = PI / 2.0
		var shin := _capsule(0.1, 0.6, dark)
		shin.position = Vector3(x, -0.25, 0.5)


func _capsule(radius: float, height: float, mat: Material) -> MeshInstance3D:
	var mesh := CapsuleMesh.new()
	mesh.radius = radius
	mesh.height = height
	var part := MeshInstance3D.new()
	part.mesh = mesh
	part.material_override = mat
	_legs.add_child(part)
	return part
