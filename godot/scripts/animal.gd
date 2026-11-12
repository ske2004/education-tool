extends Node3D
## Brings an animal built by animals.gd to life. Animals in a place wander
## around where the teacher put them, peck or graze, and react to the
## student: shy ones scatter, dogs come over wagging. Talking animals (story
## characters) stay put and turn to whoever talks to them, like people do.

## What each animal does: walking speed (m/s), how it moves ("walk", "hop",
## "waddle" or "" to stay put), how close the student can come before it
## runs off (0 = never), whether it comes over to say hello, and what it
## does while standing ("peck", "graze", "sniff" or "look").
const SPECIES := {
	"dog": {"speed": 2.2, "gait": "walk", "shy": 0.0, "friendly": true, "idle": "sniff"},
	"cat": {"speed": 1.4, "gait": "walk", "shy": 1.6, "friendly": false, "idle": "look"},
	"chicken": {"speed": 1.1, "gait": "waddle", "shy": 2.2, "friendly": false, "idle": "peck"},
	"duck": {"speed": 0.9, "gait": "waddle", "shy": 2.0, "friendly": false, "idle": "peck"},
	"sheep": {"speed": 0.7, "gait": "walk", "shy": 2.4, "friendly": false, "idle": "graze"},
	"cow": {"speed": 0.5, "gait": "walk", "shy": 0.0, "friendly": false, "idle": "graze"},
	"rabbit": {"speed": 1.8, "gait": "hop", "shy": 3.0, "friendly": false, "idle": "sniff"},
	"fox": {"speed": 1.9, "gait": "walk", "shy": 3.5, "friendly": false, "idle": "sniff"},
	"owl": {"speed": 0.0, "gait": "", "shy": 0.0, "friendly": false, "idle": "look"},
}

## How far (metres) an animal strays from where it was placed.
const ROAM := 3.5

var kind := "dog"
## Whether it walks around. Off in the editor and for talking animals.
var wander := false
## Keeps it inside the place: |x| and |z| stay below this.
var bounds := 1000.0
## Its collider, if any; ignored when checking the way ahead.
var collider: CollisionObject3D
var moving := false
var talking := false

var _species: Dictionary
var _home: Vector3
var _target: Vector3
var _wait := 0.0
var _speed_boost := 1.0
var _excited := false
var _t := randf() * TAU
var _yaw_target := NAN
var _body: Node3D
var _head: Node3D
var _tail: Node3D
var _student: Node3D
var _look_timer := 0.0


func _ready() -> void:
	_species = SPECIES.get(kind, SPECIES["dog"])
	_body = $Body
	_head = $Body/Head
	_tail = $Body/Tail
	_home = global_position
	_target = _home
	_wait = randf_range(0.5, 3.0)


func face(point: Vector3) -> void:
	var d := point - global_position
	if Vector2(d.x, d.z).length() > 0.01:
		_yaw_target = atan2(d.x, d.z)


func face_yaw(yaw: float) -> void:
	_yaw_target = yaw


func _physics_process(delta: float) -> void:
	moving = false
	_excited = false
	if not wander or talking or _species["speed"] <= 0.0:
		return
	_look_timer -= delta
	if _look_timer <= 0.0:
		_look_timer = 0.25
		_student = get_tree().get_first_node_in_group("student")
		_react()
	var to := _target - global_position
	to.y = 0.0
	if to.length() < 0.15:
		_speed_boost = 1.0
		_wait -= delta
		if _wait <= 0.0:
			_pick_target()
		return
	var step := minf(to.length(), _species["speed"] * _speed_boost * delta)
	global_position += to.normalized() * step
	moving = true
	face_yaw(atan2(to.x, to.z))


## Runs from a student who comes too close, or (dogs) goes to say hello.
func _react() -> void:
	if _student == null:
		return
	var away := global_position - _student.global_position
	away.y = 0.0
	var distance := away.length()
	if _species["shy"] > 0.0 and distance < _species["shy"]:
		var flee := global_position + away.normalized() * 2.5
		# Don't run off for good: stay near home.
		if flee.distance_to(_home) > ROAM * 1.5:
			flee = _home + (flee - _home).normalized() * ROAM * 1.5
		_target = _clear_way(flee)
		_speed_boost = 2.2
		_wait = randf_range(1.0, 2.5)
	elif _species["friendly"] and distance < 7.0:
		_excited = true
		if distance > 1.6:
			_target = _clear_way(_student.global_position + away.normalized() * 1.3)
			_speed_boost = 1.4
			_wait = 1.5


func _pick_target() -> void:
	for attempt in 5:
		var angle := randf() * TAU
		var spot := _home + Vector3(cos(angle), 0, sin(angle)) * randf_range(0.8, ROAM)
		var clear := _clear_way(spot)
		if clear.distance_to(global_position) > 0.6:
			_target = clear
			break
	_wait = randf_range(1.5, 5.0)


## How far towards `spot` the animal can walk before bumping into something
## (houses, water, the student), and kept inside the place.
func _clear_way(spot: Vector3) -> Vector3:
	var limit := bounds - 0.8
	spot.x = clampf(spot.x, -limit, limit)
	spot.z = clampf(spot.z, -limit, limit)
	spot.y = global_position.y
	if not is_inside_tree():
		return spot
	var from := global_position + Vector3(0, 0.3, 0)
	var query := PhysicsRayQueryParameters3D.create(from, spot + Vector3(0, 0.3, 0))
	if collider:
		query.exclude = [collider.get_rid()]
	var hit := get_world_3d().direct_space_state.intersect_ray(query)
	if hit.is_empty():
		return spot
	var back: Vector3 = hit["position"] - from
	back.y = 0.0
	return global_position + back.normalized() * maxf(0.0, back.length() - 0.6)


func _process(delta: float) -> void:
	var settle := minf(1.0, delta * 8.0)
	_t += delta
	var speed: float = _species.get("speed", 1.0) * _speed_boost
	var bob := 0.0
	var roll := 0.0
	var tilt := 0.0
	var head_turn := 0.0
	var nod := 0.0
	if moving:
		var pace := _t * (6.0 + speed * 4.0)
		match _species["gait"]:
			"hop":
				bob = absf(sin(pace * 0.7)) * 0.18
				tilt = -cos(pace * 0.7) * 0.15
			"waddle":
				bob = absf(sin(pace)) * 0.03
				roll = sin(pace) * 0.18
			_:
				bob = absf(sin(pace)) * 0.04
				roll = sin(pace) * 0.04
	elif talking:
		nod = absf(sin(_t * 6.0)) * 0.15
	else:
		var cycle := fmod(_t + float(get_instance_id() % 97), 6.0)
		match _species["idle"]:
			"peck":
				tilt = 0.55 if cycle < 0.8 and fmod(cycle, 0.4) < 0.2 else 0.0
			"graze":
				tilt = 0.32 if cycle < 3.5 else 0.0
			"sniff":
				tilt = 0.18 if cycle < 1.0 else 0.0
				head_turn = sin(_t * 1.3) * 0.25
			"look":
				head_turn = 0.6 if cycle < 1.5 else (-0.6 if cycle > 3.5 and cycle < 4.5 else 0.0)
	_body.position.y = lerpf(_body.position.y, bob, settle * 2.0)
	_body.rotation.z = lerpf(_body.rotation.z, roll, settle)
	_body.rotation.x = lerpf(_body.rotation.x, tilt, settle)
	_head.rotation.y = lerpf(_head.rotation.y, head_turn, settle * 0.5)
	_head.rotation.x = lerpf(_head.rotation.x, nod, settle)
	if _tail:
		var wag := 14.0 if _excited else 3.0
		_tail.rotation.y = sin(_t * wag) * (0.7 if _excited else 0.25)
	if not is_nan(_yaw_target):
		global_rotation.y = lerp_angle(global_rotation.y, _yaw_target, minf(1.0, delta * 7.0))
