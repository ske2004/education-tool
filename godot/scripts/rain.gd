extends CPUParticles3D
## Rain that falls around the camera, so a big place doesn't need a cloud of
## particles over all of it.

const AREA := 18.0
## Where drops start, in metres above the ground.
const HEIGHT := 14.0


func _init() -> void:
	amount = 1400
	lifetime = 1.1
	preprocess = 1.1
	local_coords = false
	emission_shape = CPUParticles3D.EMISSION_SHAPE_BOX
	emission_box_extents = Vector3(AREA, 0.5, AREA)
	direction = Vector3(0.12, -1.0, 0.0)
	spread = 2.0
	initial_velocity_min = 16.0
	initial_velocity_max = 19.0
	gravity = Vector3(0, -9.8, 0)
	var drop := BoxMesh.new()
	drop.size = Vector3(0.025, 0.55, 0.025)
	var material := StandardMaterial3D.new()
	material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	material.albedo_color = Color(0.78, 0.86, 1.0, 0.45)
	material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	# Drops right in front of the camera would be big streaks; fade them out.
	material.distance_fade_mode = BaseMaterial3D.DISTANCE_FADE_PIXEL_ALPHA
	material.distance_fade_min_distance = 2.0
	material.distance_fade_max_distance = 7.0
	drop.material = material
	mesh = drop


func _process(_delta: float) -> void:
	var camera := get_viewport().get_camera_3d()
	if camera == null:
		return
	# Centre the rain a little ahead of the camera, where the student looks.
	var ahead := -camera.global_basis.z
	ahead.y = 0.0
	var at := camera.global_position + ahead.normalized() * 8.0
	global_position = Vector3(at.x, HEIGHT, at.z)
