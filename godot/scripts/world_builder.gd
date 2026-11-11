extends RefCounted
## Builds the 3D scene for one place of a story out of simple shapes.

const StoryData = preload("res://scripts/story.gd")
const Actor = preload("res://scripts/actor.gd")
const Spinner = preload("res://scripts/spinner.gd")

const SKIN := Color("#f1c7a1")
const WOOD := Color("#6b4a2f")
const ROOF := Color("#b4553d")
const WALL_HEIGHT := 3.0
## Physics layer bit for walls. The player collides with them; the camera doesn't.
const WALL_LAYER := 2

static var _materials := {}


## Builds a place. Items in `collected` are left out. With `editing`, the
## start point is shown too.
##
## The returned node carries two pieces of metadata:
## - "entities": {"prop:3": node, "door:0": node, "character:<id>": node,
##   "item:<id>": node, "start": node}
## - "interactables": [{kind, id, node, label, radius}] for play mode
## - "walls": [{node, axis, side, half}], see update_walls()
static func build_place(story: Dictionary, place_id: String, collected := {}, editing := false) -> Node3D:
	var root := Node3D.new()
	root.name = "Place"
	var entities := {}
	var interactables := []
	root.set_meta("entities", entities)
	root.set_meta("interactables", interactables)
	root.set_meta("walls", [])
	var place := StoryData.find(story, "places", place_id)
	if place.is_empty():
		return root

	var indoor: bool = place["indoor"]
	var size: float = place["size"]
	_add_environment(root, indoor, size, Color(place["sky"]), Color(place["ground"]))
	_add_ground(root, indoor, size, Color(place["ground"]), Color(place["sky"]))

	var props: Array = place["props"]
	for i in props.size():
		var node := make_prop(props[i]["kind"], Color(props[i]["color"]))
		_put(node, props[i])
		root.add_child(node)
		entities["prop:%d" % i] = node

	var doors: Array = place["doors"]
	for i in doors.size():
		var door: Dictionary = doors[i]
		var target := StoryData.find(story, "places", door["to"])
		var title: String = door["label"] if door["label"] != "" else target.get("name", "Nowhere yet")
		var node := make_door(title)
		_put(node, door)
		root.add_child(node)
		entities["door:%d" % i] = node
		interactables.append({"kind": "door", "id": i, "to": door["to"], "node": node,
				"label": "Go to %s" % title, "radius": 2.0})

	for character in story["characters"]:
		if character["place"] != place_id:
			continue
		var node := make_person(Color(character["color"]), character["name"], true)
		_put(node, character)
		root.add_child(node)
		entities["character:" + character["id"]] = node
		interactables.append({"kind": "character", "id": character["id"], "node": node,
				"label": "Talk to %s" % character["name"], "radius": 2.3})

	for item in story["items"]:
		if item["place"] != place_id or collected.has(item["id"]):
			continue
		var node := make_item(Color(item["color"]), item["name"])
		_put(node, item)
		root.add_child(node)
		entities["item:" + item["id"]] = node
		interactables.append({"kind": "item", "id": item["id"], "node": node,
				"label": "Pick up %s" % item["name"], "radius": 1.8})

	if editing and story["start_place"] == place_id:
		var marker := Node3D.new()
		marker.add_child(make_ring(0.6, Color("#4a7fd4")))
		label(marker, "Start", 0.6, 36, Color("#dce9ff"))
		marker.position = Vector3(story["start_pos"][0], 0.0, story["start_pos"][1])
		root.add_child(marker)
		entities["start"] = marker
	return root


static func make_prop(kind: String, color: Color) -> StaticBody3D:
	var body := StaticBody3D.new()
	match kind:
		"house":
			box(body, Vector3(4, 3, 4), Vector3(0, 1.5, 0), color)
			var roof := PrismMesh.new()
			roof.size = Vector3(4.6, 1.6, 4.6)
			_add_mesh(body, roof, Vector3(0, 3.8, 0), ROOF)
			box(body, Vector3(0.5, 1.2, 0.5), Vector3(1.2, 4.3, -0.9), Color("#8d7e72"))
			box(body, Vector3(1.0, 1.8, 0.08), Vector3(0, 0.9, 2.02), Color("#6b4226"))
			for x in [-1.3, 1.3]:
				box(body, Vector3(0.8, 0.8, 0.06), Vector3(x, 1.9, 2.02), Color("#ffe6a3"), 0.6)
			_box_shape(body, Vector3(4, 3, 4), Vector3(0, 1.5, 0))
		"tree":
			cylinder(body, 0.18, 0.26, 1.4, Vector3(0, 0.7, 0), Color("#7a5230"))
			sphere(body, 1.1, Vector3(0, 2.2, 0), color)
			sphere(body, 0.75, Vector3(0.55, 1.9, 0.3), color.darkened(0.08))
			sphere(body, 0.8, Vector3(-0.5, 2.0, -0.3), color.lightened(0.06))
			_cylinder_shape(body, 0.35, 3.0, Vector3(0, 1.5, 0))
		"pine":
			cylinder(body, 0.15, 0.2, 1.0, Vector3(0, 0.5, 0), Color("#6e4a2b"))
			cylinder(body, 0.0, 1.1, 1.6, Vector3(0, 1.6, 0), color)
			cylinder(body, 0.0, 0.85, 1.3, Vector3(0, 2.4, 0), color.lightened(0.05))
			cylinder(body, 0.0, 0.55, 1.0, Vector3(0, 3.1, 0), color.lightened(0.1))
			_cylinder_shape(body, 0.4, 3.5, Vector3(0, 1.75, 0))
		"rock":
			var big := sphere(body, 0.8, Vector3(0, 0.3, 0), color)
			big.scale = Vector3(1.3, 0.75, 1.0)
			var small := sphere(body, 0.45, Vector3(0.75, 0.2, 0.35), color.darkened(0.1))
			small.scale = Vector3(1.0, 0.8, 1.1)
			_box_shape(body, Vector3(2.0, 1.0, 1.6), Vector3(0, 0.5, 0))
		"fence":
			for x in [-1.4, 0.0, 1.4]:
				box(body, Vector3(0.14, 1.0, 0.14), Vector3(x, 0.5, 0), color.darkened(0.15))
			for y in [0.45, 0.8]:
				box(body, Vector3(3.0, 0.12, 0.08), Vector3(0, y, 0), color)
			_box_shape(body, Vector3(3.0, 1.0, 0.25), Vector3(0, 0.5, 0))
		"lamp":
			cylinder(body, 0.18, 0.22, 0.2, Vector3(0, 0.1, 0), color)
			cylinder(body, 0.06, 0.06, 2.7, Vector3(0, 1.45, 0), color)
			sphere(body, 0.22, Vector3(0, 2.9, 0), Color("#ffe2a0"), 2.0)
			var light := OmniLight3D.new()
			light.position = Vector3(0, 2.9, 0)
			light.omni_range = 7.0
			light.light_energy = 0.8
			light.light_color = Color("#ffd89a")
			body.add_child(light)
			_cylinder_shape(body, 0.2, 3.0, Vector3(0, 1.5, 0))
		"bench":
			box(body, Vector3(1.8, 0.1, 0.5), Vector3(0, 0.5, 0), color)
			box(body, Vector3(1.8, 0.45, 0.08), Vector3(0, 0.8, -0.22), color)
			for x in [-0.75, 0.75]:
				box(body, Vector3(0.1, 0.5, 0.45), Vector3(x, 0.25, 0), color.darkened(0.3))
			_box_shape(body, Vector3(1.8, 1.0, 0.6), Vector3(0, 0.5, 0))
		"well":
			cylinder(body, 1.0, 1.05, 0.9, Vector3(0, 0.45, 0), color)
			cylinder(body, 0.8, 0.8, 0.04, Vector3(0, 0.91, 0), Color("#33404d"))
			for x in [-0.9, 0.9]:
				box(body, Vector3(0.14, 2.1, 0.14), Vector3(x, 1.05, 0), WOOD)
			var roof := PrismMesh.new()
			roof.size = Vector3(2.5, 0.8, 1.6)
			_add_mesh(body, roof, Vector3(0, 2.45, 0), ROOF)
			_cylinder_shape(body, 1.05, 2.0, Vector3(0, 1.0, 0))
		"crate":
			box(body, Vector3(0.9, 0.9, 0.9), Vector3(0, 0.45, 0), color)
			box(body, Vector3(0.94, 0.12, 0.94), Vector3(0, 0.75, 0), color.darkened(0.2))
			box(body, Vector3(0.94, 0.12, 0.94), Vector3(0, 0.15, 0), color.darkened(0.2))
			_box_shape(body, Vector3(0.9, 0.9, 0.9), Vector3(0, 0.45, 0))
		"table":
			box(body, Vector3(2.0, 0.12, 1.1), Vector3(0, 0.8, 0), color)
			for x in [-0.85, 0.85]:
				for z in [-0.42, 0.42]:
					box(body, Vector3(0.1, 0.75, 0.1), Vector3(x, 0.38, z), color.darkened(0.25))
			_box_shape(body, Vector3(2.0, 0.9, 1.1), Vector3(0, 0.45, 0))
		"shelf":
			box(body, Vector3(2.2, 2.4, 0.5), Vector3(0, 1.2, 0), color)
			var colors := ["#c0504d", "#4f81bd", "#9bbb59", "#f2c14e", "#8064a2", "#4bacc6"]
			for row in 3:
				for b in 7:
					var h := 0.42 + 0.08 * ((b * 7 + row * 3) % 3)
					box(body, Vector3(0.22, h, 0.36), Vector3(-0.85 + b * 0.28, 0.2 + row * 0.75 + h / 2.0, 0.1),
							Color(colors[(b + row * 2) % colors.size()]))
			_box_shape(body, Vector3(2.2, 2.4, 0.5), Vector3(0, 1.2, 0))
		"plant":
			cylinder(body, 0.3, 0.22, 0.5, Vector3(0, 0.25, 0), Color("#c46a45"))
			sphere(body, 0.45, Vector3(0, 0.85, 0), color)
			sphere(body, 0.3, Vector3(0.25, 1.15, 0.1), color.lightened(0.08))
			_cylinder_shape(body, 0.3, 1.2, Vector3(0, 0.6, 0))
		_:
			box(body, Vector3.ONE, Vector3(0, 0.5, 0), color)
			_box_shape(body, Vector3.ONE, Vector3(0, 0.5, 0))
	return body


static func make_door(title: String) -> Node3D:
	var root := Node3D.new()
	for x in [-0.75, 0.75]:
		box(root, Vector3(0.16, 2.4, 0.16), Vector3(x, 1.2, 0), WOOD)
	box(root, Vector3(1.66, 0.16, 0.16), Vector3(0, 2.45, 0), WOOD)
	box(root, Vector3(1.34, 2.3, 0.04), Vector3(0, 1.15, 0), Color("#ffd27a"), 1.2, 0.45)
	label(root, title, 3.0, 36, Color("#fff4d6"))
	return root


## A person. `solid` adds a collider so students can't walk through them.
static func make_person(color: Color, title := "", solid := false) -> Node3D:
	var root: Node3D = Actor.new()
	var body := Node3D.new()
	body.name = "Body"
	root.add_child(body)
	var torso := CapsuleMesh.new()
	torso.radius = 0.32
	torso.height = 1.15
	_add_mesh(body, torso, Vector3(0, 0.72, 0), color)
	sphere(body, 0.27, Vector3(0, 1.52, 0), SKIN)
	sphere(body, 0.28, Vector3(0, 1.6, -0.06), color.darkened(0.55))
	for x in [-0.1, 0.1]:
		sphere(body, 0.045, Vector3(x, 1.55, 0.24), Color("#2b2b2b"))
	if title != "":
		label(root, title, 2.15, 44)
	if solid:
		var collider := StaticBody3D.new()
		_cylinder_shape(collider, 0.4, 1.8, Vector3(0, 0.9, 0))
		root.add_child(collider)
	return root


static func make_item(color: Color, title := "") -> Node3D:
	var root: Node3D = Spinner.new()
	var gem := Node3D.new()
	gem.name = "Gem"
	gem.position.y = Spinner.HEIGHT
	root.add_child(gem)
	var cube := box(gem, Vector3(0.34, 0.34, 0.34), Vector3.ZERO, color, 0.5)
	cube.rotation_degrees = Vector3(35.0, 0.0, 45.0)
	var light := OmniLight3D.new()
	light.light_color = color
	light.omni_range = 2.0
	light.light_energy = 0.6
	gem.add_child(light)
	if title != "":
		label(root, title, 1.75, 30)
	return root


static func make_ring(radius: float, color: Color) -> MeshInstance3D:
	var torus := TorusMesh.new()
	torus.inner_radius = radius - 0.08
	torus.outer_radius = radius
	var mesh := MeshInstance3D.new()
	mesh.mesh = torus
	mesh.material_override = material(color, 1.5)
	mesh.position.y = 0.05
	return mesh


static func material(color: Color, emission := 0.0, alpha := 1.0) -> StandardMaterial3D:
	var key := "%s/%s/%s" % [color.to_html(), emission, alpha]
	if _materials.has(key):
		return _materials[key]
	var m := StandardMaterial3D.new()
	m.albedo_color = Color(color, alpha)
	m.roughness = 0.85
	if emission > 0.0:
		m.emission_enabled = true
		m.emission = color
		m.emission_energy_multiplier = emission
	if alpha < 1.0:
		m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	_materials[key] = m
	return m


static func box(parent: Node3D, size: Vector3, pos: Vector3, color: Color, emission := 0.0, alpha := 1.0) -> MeshInstance3D:
	var mesh := BoxMesh.new()
	mesh.size = size
	return _add_mesh(parent, mesh, pos, color, emission, alpha)


static func cylinder(parent: Node3D, top: float, bottom: float, height: float, pos: Vector3, color: Color) -> MeshInstance3D:
	var mesh := CylinderMesh.new()
	mesh.top_radius = top
	mesh.bottom_radius = bottom
	mesh.height = height
	return _add_mesh(parent, mesh, pos, color)


static func sphere(parent: Node3D, radius: float, pos: Vector3, color: Color, emission := 0.0) -> MeshInstance3D:
	var mesh := SphereMesh.new()
	mesh.radius = radius
	mesh.height = radius * 2.0
	return _add_mesh(parent, mesh, pos, color, emission)


static func label(parent: Node3D, text: String, y: float, size := 40, color := Color.WHITE) -> Label3D:
	var l := Label3D.new()
	l.text = text
	l.position.y = y
	l.font_size = size
	l.pixel_size = 0.0065
	l.outline_size = 12
	l.outline_modulate = Color(0, 0, 0, 0.55)
	l.modulate = color
	l.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	parent.add_child(l)
	return l


static func _add_mesh(parent: Node3D, mesh: Mesh, pos: Vector3, color: Color, emission := 0.0, alpha := 1.0) -> MeshInstance3D:
	var instance := MeshInstance3D.new()
	instance.mesh = mesh
	instance.material_override = material(color, emission, alpha)
	instance.position = pos
	parent.add_child(instance)
	return instance


static func _box_shape(body: CollisionObject3D, size: Vector3, pos: Vector3) -> void:
	var shape := BoxShape3D.new()
	shape.size = size
	var collision := CollisionShape3D.new()
	collision.shape = shape
	collision.position = pos
	body.add_child(collision)


static func _cylinder_shape(body: CollisionObject3D, radius: float, height: float, pos: Vector3) -> void:
	var shape := CylinderShape3D.new()
	shape.radius = radius
	shape.height = height
	var collision := CollisionShape3D.new()
	collision.shape = shape
	collision.position = pos
	body.add_child(collision)


static func _put(node: Node3D, entry: Dictionary) -> void:
	node.position = Vector3(entry["pos"][0], 0.0, entry["pos"][1])
	node.rotation.y = deg_to_rad(entry.get("rot", 0.0))


static func _add_environment(root: Node3D, indoor: bool, size: float, sky: Color, ground: Color) -> void:
	var env := Environment.new()
	if indoor:
		env.background_mode = Environment.BG_COLOR
		env.background_color = sky.darkened(0.8)
		env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
		env.ambient_light_color = sky.lerp(Color.WHITE, 0.3)
		env.ambient_light_energy = 0.55
	else:
		var sky_material := ProceduralSkyMaterial.new()
		sky_material.sky_top_color = sky.darkened(0.25)
		sky_material.sky_horizon_color = sky.lerp(Color.WHITE, 0.55)
		sky_material.ground_horizon_color = sky.lerp(Color.WHITE, 0.55)
		sky_material.ground_bottom_color = ground.darkened(0.5)
		var sky_resource := Sky.new()
		sky_resource.sky_material = sky_material
		env.background_mode = Environment.BG_SKY
		env.sky = sky_resource
		env.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
		env.ambient_light_energy = 0.9
		env.fog_enabled = true
		env.fog_light_color = sky.lerp(Color.WHITE, 0.55)
		env.fog_density = 0.0025
	env.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	env.ssao_enabled = true
	env.glow_enabled = true
	var world_environment := WorldEnvironment.new()
	world_environment.environment = env
	root.add_child(world_environment)

	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-50, -35, 0)
	sun.shadow_enabled = true
	sun.light_energy = 0.35 if indoor else 1.1
	sun.light_color = Color("#fff4e0")
	root.add_child(sun)
	if indoor:
		var lamp := OmniLight3D.new()
		lamp.position = Vector3(0, WALL_HEIGHT + 0.5, 0)
		lamp.omni_range = size
		lamp.light_energy = 1.4
		lamp.light_color = Color("#ffe1b0")
		lamp.shadow_enabled = true
		root.add_child(lamp)


static func _add_ground(root: Node3D, indoor: bool, size: float, ground: Color, walls: Color) -> void:
	var floor_body := StaticBody3D.new()
	floor_body.name = "Ground"
	var plane := PlaneMesh.new()
	plane.size = Vector2(size, size)
	_add_mesh(floor_body, plane, Vector3.ZERO, ground)
	_box_shape(floor_body, Vector3(size + 2.0, 1.0, size + 2.0), Vector3(0, -0.5, 0))
	root.add_child(floor_body)
	if not indoor:
		var outer := PlaneMesh.new()
		outer.size = Vector2(size * 6.0, size * 6.0)
		_add_mesh(root, outer, Vector3(0, -0.02, 0), ground.darkened(0.12))
	# Walls keep students inside; outdoors they're invisible. They sit on
	# their own physics layer so the camera can swing over them.
	var half := size / 2.0
	for side in 4:
		var along_x := side % 2 == 0
		var dims := Vector3(size + 0.6, WALL_HEIGHT, 0.3) if along_x else Vector3(0.3, WALL_HEIGHT, size + 0.6)
		var direction := -1.0 if side < 2 else 1.0
		var wall := StaticBody3D.new()
		wall.collision_layer = WALL_LAYER
		wall.position = Vector3(0, WALL_HEIGHT / 2.0, direction * half) if along_x else Vector3(direction * half, WALL_HEIGHT / 2.0, 0)
		_box_shape(wall, dims, Vector3.ZERO)
		if indoor:
			var mesh := box(wall, dims, Vector3.ZERO, walls)
			root.get_meta("walls").append({"node": mesh, "axis": 2 if along_x else 0, "side": direction, "half": half})
		root.add_child(wall)


## Hides the walls between the camera and the room, so indoors the camera
## looks in over the near wall instead of at it.
static func update_walls(place: Node3D, camera_position: Vector3) -> void:
	if place == null:
		return
	for wall in place.get_meta("walls", []):
		var outside: bool = camera_position[wall["axis"]] * wall["side"] > wall["half"] - 0.5
		wall["node"].visible = not outside
