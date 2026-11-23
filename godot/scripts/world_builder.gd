extends RefCounted
## Builds the 3D scene for one place of a story out of simple shapes.

const StoryData = preload("res://scripts/story.gd")
const Actor = preload("res://scripts/actor.gd")
const Spinner = preload("res://scripts/spinner.gd")
const Drifter = preload("res://scripts/drifter.gd")
const Rain = preload("res://scripts/rain.gd")
const WaterShader = preload("res://shaders/water.gdshader")
const Animals = preload("res://scripts/animals.gd")

const SKIN := Color("#f1c7a1")
const WOOD := Color("#6b4a2f")
const ROOF := Color("#b4553d")
const STONE := Color("#a9a297")
const SAND := Color("#d8c48f")
## How far a river section runs, so sections placed this far apart join up.
const RIVER_LENGTH := 8.0
const RIVER_WIDTH := 3.0

## Per weather: clouds in the sky, their colour, how much the sky greys, the
## sun's strength and the fog.
const WEATHER := {
	"clear": {"clouds": 6, "cloud": "#ffffff", "grey": 0.0, "sun": 1.1, "fog": 0.0025},
	"cloudy": {"clouds": 16, "cloud": "#e4e7ec", "grey": 0.4, "sun": 0.75, "fog": 0.004},
	"rain": {"clouds": 22, "cloud": "#8f969f", "grey": 0.7, "sun": 0.4, "fog": 0.011},
}
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
	var weather: Dictionary = WEATHER[place.get("weather", "clear")]
	_add_environment(root, indoor, size, Color(place["sky"]), Color(place["ground"]), weather)
	_add_ground(root, indoor, size, Color(place["ground"]), Color(place["sky"]))
	if not indoor:
		_add_clouds(root, size, weather, place["id"])
		if place.get("weather", "clear") == "rain":
			root.add_child(Rain.new())

	var props: Array = place["props"]
	for i in props.size():
		var node := make_prop(props[i]["kind"], Color(props[i]["color"]))
		_put(node, props[i])
		root.add_child(node)
		entities["prop:%d" % i] = node
		if props[i]["kind"] == "bench":
			interactables.append({"kind": "seat", "id": i, "node": node,
					"label": "Sit on the bench", "radius": 1.6})

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
		var node: Node3D
		if character["look"] == "person":
			node = make_person(Color(character["color"]), character["name"], true)
		else:
			node = Animals.make(character["look"], Color(character["color"]), character["name"], true)
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

	# Animals stand still in the editor, so they stay where the teacher puts them.
	if not editing:
		for key in entities:
			var animal = entities[key].get_node_or_null("Animal")
			if animal:
				animal.wander = true
				animal.bounds = size / 2.0

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
	if StoryData.ANIMALS.has(kind):
		var animal := Animals.make(kind, color)
		animal.name = "Animal"
		body.add_child(animal)
		return body
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
		"pond":
			var bed := cylinder(body, 2.5, 2.5, 0.1, Vector3(0, 0.05, 0), color.darkened(0.6))
			bed.scale.z = 0.8
			var water := cylinder(body, 2.4, 2.4, 0.02, Vector3(0, 0.14, 0), color)
			water.scale.z = 0.8
			water.material_override = water_material(color)
			var ring := TorusMesh.new()
			ring.inner_radius = 2.3
			ring.outer_radius = 2.8
			var rim := _add_mesh(body, ring, Vector3(0, 0.12, 0), STONE)
			rim.scale = Vector3(1.0, 0.6, 0.8)
			for reed in [Vector3(-2.2, 0, -0.9), Vector3(-1.9, 0, -1.3), Vector3(2.3, 0, 0.6), Vector3(1.6, 0, 1.6)]:
				cylinder(body, 0.03, 0.05, 1.1, reed + Vector3(0, 0.55, 0), Color("#5f8a3c"))
				cylinder(body, 0.07, 0.07, 0.3, reed + Vector3(0, 1.0, 0), Color("#7a5230"))
			var collider := _cylinder_shape(body, 2.6, 0.8, Vector3(0, 0.4, 0))
			collider.scale.z = 0.8
		"river":
			var surface := PlaneMesh.new()
			surface.size = Vector2(RIVER_WIDTH, RIVER_LENGTH)
			var water := _add_mesh(body, surface, Vector3(0, 0.06, 0), color)
			water.material_override = water_material(color, 0.9)
			for x in [-1.0, 1.0]:
				box(body, Vector3(0.5, 0.08, RIVER_LENGTH), Vector3(x * (RIVER_WIDTH / 2.0 + 0.2), 0.04, 0), SAND)
			# Low enough that a bridge deck clears it.
			_box_shape(body, Vector3(RIVER_WIDTH, 0.4, RIVER_LENGTH), Vector3(0, 0.2, 0))
		"bridge":
			_make_bridge(body, color)
		"fountain":
			cylinder(body, 1.4, 1.45, 0.6, Vector3(0, 0.3, 0), color)
			var basin := cylinder(body, 1.2, 1.2, 0.02, Vector3(0, 0.56, 0), Color("#4a9bd6"))
			basin.material_override = water_material(Color("#4a9bd6"))
			cylinder(body, 0.18, 0.24, 1.3, Vector3(0, 1.0, 0), color.darkened(0.1))
			cylinder(body, 0.55, 0.3, 0.2, Vector3(0, 1.7, 0), color)
			body.add_child(_spray())
			_cylinder_shape(body, 1.45, 1.0, Vector3(0, 0.5, 0))
		"plant":
			cylinder(body, 0.3, 0.22, 0.5, Vector3(0, 0.25, 0), Color("#c46a45"))
			sphere(body, 0.45, Vector3(0, 0.85, 0), color)
			sphere(body, 0.3, Vector3(0.25, 1.15, 0.1), color.lightened(0.08))
			_cylinder_shape(body, 0.3, 1.2, Vector3(0, 0.6, 0))
		_:
			box(body, Vector3.ONE, Vector3(0, 0.5, 0), color)
			_box_shape(body, Vector3.ONE, Vector3(0, 0.5, 0))
	return body


## A footbridge across a river section: ramps up to a deck above the water,
## with rails so students stay on it. It spans x, the river's width.
static func _make_bridge(body: StaticBody3D, color: Color) -> void:
	const DECK := 0.6
	const HALF := 1.7
	const RAMP := 1.3
	const WIDTH := 1.6
	box(body, Vector3(HALF * 2.0, 0.14, WIDTH), Vector3(0, DECK - 0.07, 0), color)
	_box_shape(body, Vector3(HALF * 2.0, 0.14, WIDTH), Vector3(0, DECK - 0.07, 0))
	var slope := atan2(DECK, RAMP)
	var ramp_length := sqrt(DECK * DECK + RAMP * RAMP)
	for side in [-1.0, 1.0]:
		var ramp := Node3D.new()
		ramp.position = Vector3(side * (HALF + RAMP / 2.0), DECK / 2.0 - 0.07, 0)
		ramp.rotation.z = -slope * side
		body.add_child(ramp)
		box(ramp, Vector3(ramp_length, 0.14, WIDTH), Vector3.ZERO, color)
		var collision := _box_shape(body, Vector3(ramp_length, 0.14, WIDTH), ramp.position)
		collision.rotation = ramp.rotation
		for z in [-1.0, 1.0]:
			box(body, Vector3(0.12, 1.0, 0.12), Vector3(side * HALF, DECK + 0.5, z * (WIDTH / 2.0 - 0.06)), color.darkened(0.25))
	for z in [-1.0, 1.0]:
		var at := Vector3(0, DECK + 0.95, z * (WIDTH / 2.0 - 0.06))
		box(body, Vector3(HALF * 2.0, 0.1, 0.1), at, color.darkened(0.15))
		_box_shape(body, Vector3(HALF * 2.0 + RAMP, 1.0, 0.12), Vector3(0, DECK + 0.5, at.z))


## Droplets thrown up from a fountain's top bowl.
static func _spray() -> CPUParticles3D:
	var spray := CPUParticles3D.new()
	spray.position = Vector3(0, 1.85, 0)
	spray.amount = 160
	spray.lifetime = 1.0
	spray.direction = Vector3.UP
	spray.spread = 16.0
	spray.initial_velocity_min = 3.2
	spray.initial_velocity_max = 3.8
	spray.scale_amount_min = 0.6
	spray.scale_amount_max = 1.2
	var drop := SphereMesh.new()
	drop.radius = 0.04
	drop.height = 0.08
	drop.radial_segments = 6
	drop.rings = 3
	drop.material = material(Color("#d6ecff"), 0.4, 0.8)
	spray.mesh = drop
	return spray


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


## Animated water. `flow` makes the ripples run along the mesh, for rivers.
static func water_material(color: Color, flow := 0.0) -> ShaderMaterial:
	var key := "water/%s/%s" % [color.to_html(), flow]
	if _materials.has(key):
		return _materials[key]
	var m := ShaderMaterial.new()
	m.shader = WaterShader
	m.set_shader_parameter("deep", color.darkened(0.25))
	m.set_shader_parameter("shallow", color.lightened(0.35))
	m.set_shader_parameter("flow", flow)
	_materials[key] = m
	return m


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


static func _box_shape(body: CollisionObject3D, size: Vector3, pos: Vector3) -> CollisionShape3D:
	var shape := BoxShape3D.new()
	shape.size = size
	var collision := CollisionShape3D.new()
	collision.shape = shape
	collision.position = pos
	body.add_child(collision)
	return collision


static func _cylinder_shape(body: CollisionObject3D, radius: float, height: float, pos: Vector3) -> CollisionShape3D:
	var shape := CylinderShape3D.new()
	shape.radius = radius
	shape.height = height
	var collision := CollisionShape3D.new()
	collision.shape = shape
	collision.position = pos
	body.add_child(collision)
	return collision


static func _put(node: Node3D, entry: Dictionary) -> void:
	node.position = Vector3(entry["pos"][0], 0.0, entry["pos"][1])
	node.rotation.y = deg_to_rad(entry.get("rot", 0.0))


## Whether the Compatibility renderer (browsers, older machines) is drawing.
static func compatibility() -> bool:
	return RenderingServer.get_current_rendering_method() == "gl_compatibility"


static func _add_environment(root: Node3D, indoor: bool, size: float, sky: Color, ground: Color, weather: Dictionary) -> void:
	var env := Environment.new()
	if not indoor:
		sky = sky.lerp(Color("#9aa3ad"), weather["grey"])
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
		env.fog_density = weather["fog"]
	env.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	env.ssao_enabled = true
	env.glow_enabled = true
	var sun_energy: float = 0.35 if indoor else weather["sun"]
	if compatibility():
		# The Compatibility renderer lights the sky's ambient and fog far more
		# strongly, which washes everything out; tone it down to match.
		env.glow_enabled = false
		env.tonemap_mode = Environment.TONE_MAPPER_LINEAR
		if not indoor:
			env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
			env.ambient_light_color = sky.lerp(Color.WHITE, 0.45)
			env.ambient_light_energy = 0.5
			env.fog_density *= 0.15
			sun_energy *= 0.75
		else:
			env.ambient_light_energy *= 0.6
	var world_environment := WorldEnvironment.new()
	world_environment.environment = env
	root.add_child(world_environment)

	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-50, -35, 0)
	sun.shadow_enabled = true
	sun.light_energy = sun_energy
	sun.light_color = Color("#fff4e0")
	root.add_child(sun)
	if indoor:
		var lamp := OmniLight3D.new()
		lamp.position = Vector3(0, WALL_HEIGHT + 0.5, 0)
		lamp.omni_range = size
		lamp.light_energy = 0.8 if compatibility() else 1.4
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


## High clouds drifting over an outdoor place. They're placed the same way
## every time a place is built, and their shadows sweep across the ground.
static func _add_clouds(root: Node3D, size: float, weather: Dictionary, seed_text: String) -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = hash(seed_text)
	var color := Color(weather["cloud"])
	# Most clouds hang around the place rather than over it, low enough to
	# show above the horizon from the play camera.
	for i in weather["clouds"]:
		var cloud: Node3D = Drifter.new()
		cloud.limit = size * 2.0
		cloud.speed = rng.randf_range(0.4, 0.9)
		var angle := rng.randf() * TAU
		var away := rng.randf_range(size * 0.4, size * 1.8)
		cloud.position = Vector3(cos(angle) * away, rng.randf_range(12.0, 18.0), sin(angle) * away)
		var puffs := rng.randi_range(4, 7)
		for p in puffs:
			var radius := rng.randf_range(2.0, 3.6)
			var along := (p - puffs / 2.0) * 2.3
			var puff := sphere(cloud, radius, Vector3(along, rng.randf_range(-0.3, 0.6), rng.randf_range(-1.4, 1.4)), color, 0.35)
			puff.scale = Vector3(1.0, 0.62, 0.85)
		root.add_child(cloud)


## Hides the walls between the camera and the room, so indoors the camera
## looks in over the near wall instead of at it.
static func update_walls(place: Node3D, camera_position: Vector3) -> void:
	if place == null:
		return
	for wall in place.get_meta("walls", []):
		var outside: bool = camera_position[wall["axis"]] * wall["side"] > wall["half"] - 0.5
		wall["node"].visible = not outside
