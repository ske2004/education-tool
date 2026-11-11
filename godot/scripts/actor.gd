extends Node3D
## Animates a person built by WorldBuilder.make_person: breathing while idle,
## bouncing while walking, nodding while talking, and turning smoothly.

var moving := false
var talking := false
var _t := randf() * TAU
var _yaw_target := NAN
var _body: Node3D


func _ready() -> void:
	_body = get_node_or_null("Body")


func face(point: Vector3) -> void:
	var d := point - global_position
	if Vector2(d.x, d.z).length() > 0.01:
		_yaw_target = atan2(d.x, d.z)


func face_yaw(yaw: float) -> void:
	_yaw_target = yaw


func _process(delta: float) -> void:
	_t += delta * (10.0 if moving else 2.2)
	if _body:
		if moving:
			_body.position.y = absf(sin(_t)) * 0.12
			_body.rotation.z = sin(_t) * 0.07
		else:
			var nod := absf(sin(_t * 3.0)) * 0.035 if talking else 0.0
			var settle := minf(1.0, delta * 10.0)
			_body.position.y = lerpf(_body.position.y, sin(_t) * 0.015 + nod, settle)
			_body.rotation.z = lerpf(_body.rotation.z, 0.0, settle)
	if not is_nan(_yaw_target):
		rotation.y = lerp_angle(rotation.y, _yaw_target, minf(1.0, delta * 9.0))
