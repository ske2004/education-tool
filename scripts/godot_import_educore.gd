@tool
extends EditorScenePostImport
## Godot 4 Post-Import Script for Educore glTF (.glb) Worlds
##
## Attach this script in Godot's Import dock -> Import Script -> Path for any
## `world.glb` exported from Educore, then click Reimport.
##
## Features:
## - Reads scene-level `extras` (`time_of_day`, `sun_light`, `script`) and configures
##   a `DirectionalLight3D` and root scene metadata.
## - Reads node-level `extras` (`type`, `x`, `y`, `floors`, `id`, `interior_scene`,
##   `interior_node`) imported into Godot's `"extras"` node metadata dictionary.
## - Generates `StaticBody3D` + `CollisionShape3D` (`BoxShape3D`) for solid objects
##   (`building`, `castle`, `wall`, `tree`, `water`, `prop`, `ground`).
## - Creates `Area3D` door triggers at building/castle entrances with metadata
##   linking to `interior_node` and `interior_scene`.
## - Adds a `Marker3D` (`PlayerSpawn`) in group `"player_spawn"` for `player` nodes.
## - Tags `npc` and `item` nodes with interaction `Area3D` triggers and metadata.

const FLOOR_HEIGHT: float = 2.0
const BUILDING_EXTENTS: Vector3 = Vector3(8.0, 2.0, 8.0)


func _post_import(scene: Node) -> Object:
	if scene == null:
		return scene

	_apply_scene_extras(scene)

	for child in scene.get_children():
		_process_node(scene, child)

	return scene


func _apply_scene_extras(scene: Node) -> void:
	if not scene.has_meta("extras"):
		return

	var extras: Variant = scene.get_meta("extras")
	if typeof(extras) != TYPE_DICTIONARY:
		return

	var dict: Dictionary = extras
	if dict.has("time_of_day"):
		scene.set_meta("educore_time_of_day", float(dict["time_of_day"]))

	if dict.has("script"):
		scene.set_meta("educore_script", dict["script"])

	if dict.has("sun_light") and typeof(dict["sun_light"]) == TYPE_DICTIONARY:
		var sun_info: Dictionary = dict["sun_light"]
		var sun := DirectionalLight3D.new()
		sun.name = "EducoreSunLight"
		sun.shadow_enabled = true

		if sun_info.has("color") and typeof(sun_info["color"]) == TYPE_ARRAY:
			var c: Array = sun_info["color"]
			if c.size() >= 3:
				sun.light_color = Color(float(c[0]), float(c[1]), float(c[2]))

		if sun_info.has("direction") and typeof(sun_info["direction"]) == TYPE_ARRAY:
			var d: Array = sun_info["direction"]
			if d.size() >= 3:
				var dir := Vector3(float(d[0]), float(d[1]), float(d[2])).normalized()
				if dir.length_squared() > 0.0001:
					var up := Vector3.UP
					if absf(dir.dot(up)) > 0.99:
						up = Vector3.FORWARD
					sun.transform.basis = Basis.looking_at(dir, up)

		scene.add_child(sun)
		sun.owner = scene


func _process_node(root: Node, node: Node) -> void:
	if node.name == "ground" and node is Node3D:
		_add_box_collider(root, node as Node3D, Vector3(256.0, 1.0, 256.0), Vector3(0.0, -0.5, 0.0))

	if node.has_meta("extras") and typeof(node.get_meta("extras")) == TYPE_DICTIONARY:
		var extras: Dictionary = node.get_meta("extras")
		_configure_educore_object(root, node, extras)

	# Recurse into interior root nodes (e.g. when exported with single_scene_interiors)
	if node.name.contains("interior"):
		for child in node.get_children():
			_process_node(root, child)


func _configure_educore_object(root: Node, node: Node, extras: Dictionary) -> void:
	if not (node is Node3D):
		return

	var node3d := node as Node3D
	var obj_type: String = str(extras.get("type", ""))
	var floors: int = maxi(1, int(extras.get("floors", 1)))
	var obj_id: String = str(extras.get("id", ""))

	node3d.set_meta("educore_type", obj_type)
	if not obj_id.is_empty():
		node3d.set_meta("educore_id", obj_id)

	match obj_type:
		"building", "castle":
			var height: float = float(floors) * FLOOR_HEIGHT
			var size := Vector3(BUILDING_EXTENTS.x, height, BUILDING_EXTENTS.z)
			_add_box_collider(root, node3d, size, Vector3(0.0, height * 0.5, 0.0))
			_add_door_trigger(root, node3d, extras)

		"wall":
			_add_box_collider(root, node3d, Vector3(1.0, 2.0, 1.0), Vector3(0.5, 1.0, -0.5))

		"tree":
			_add_box_collider(root, node3d, Vector3(0.6, 2.5, 0.6), Vector3(0.0, 1.25, 0.0))

		"water":
			_add_box_collider(root, node3d, Vector3(1.0, 0.5, 1.0), Vector3(0.0, 0.0, 0.0))

		"prop":
			_add_box_collider(root, node3d, Vector3(0.9, 1.0, 0.9), Vector3(0.0, 0.5, 0.0))

		"player":
			var spawn := Marker3D.new()
			spawn.name = "PlayerSpawn"
			spawn.add_to_group("player_spawn", true)
			node3d.add_child(spawn)
			spawn.owner = root

		"npc", "item":
			_add_interaction_area(root, node3d, obj_type, obj_id)


func _add_box_collider(root: Node, parent: Node3D, size: Vector3, offset: Vector3) -> void:
	var body := StaticBody3D.new()
	body.name = "StaticBody3D"

	var col := CollisionShape3D.new()
	col.name = "CollisionShape3D"
	var shape := BoxShape3D.new()
	shape.size = size
	col.shape = shape
	col.position = offset

	body.add_child(col)
	parent.add_child(body)
	body.owner = root
	col.owner = root


func _add_door_trigger(root: Node, building: Node3D, extras: Dictionary) -> void:
	var door := Area3D.new()
	door.name = "DoorTrigger"
	door.add_to_group("educore_door", true)

	if extras.has("interior_scene"):
		door.set_meta("interior_scene", int(extras["interior_scene"]))
	if extras.has("interior_node"):
		door.set_meta("interior_node", int(extras["interior_node"]))

	var col := CollisionShape3D.new()
	col.name = "CollisionShape3D"
	var shape := BoxShape3D.new()
	shape.size = Vector3(1.6, 2.2, 1.2)
	col.shape = shape
	# In glTF right-handed space (mirrored Z), the building front door at z = -4
	# in editor coordinates sits at +Z = +4.0 in local glTF coordinates.
	col.position = Vector3(0.0, 1.1, 4.0)

	door.add_child(col)
	building.add_child(door)
	door.owner = root
	col.owner = root


func _add_interaction_area(root: Node, entity: Node3D, obj_type: String, obj_id: String) -> void:
	var area := Area3D.new()
	area.name = "InteractTrigger"
	area.add_to_group("educore_" + obj_type, true)
	area.set_meta("educore_type", obj_type)
	area.set_meta("educore_id", obj_id)

	var col := CollisionShape3D.new()
	col.name = "CollisionShape3D"
	var shape := BoxShape3D.new()
	shape.size = Vector3(1.5, 2.0, 1.5)
	col.shape = shape
	col.position = Vector3(0.0, 1.0, 0.0)

	area.add_child(col)
	entity.add_child(area)
	area.owner = root
	col.owner = root
