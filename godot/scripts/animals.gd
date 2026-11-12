extends RefCounted
## Builds animals out of simple shapes. Every animal faces +z and has a
## "Body" node (tilted to peck or graze), a "Head" and sometimes a "Tail",
## which animal.gd moves.

const Builder = preload("res://scripts/world_builder.gd")
const Animal = preload("res://scripts/animal.gd")

const EYE := Color("#1f1f1f")
const BEAK := Color("#f0a030")
const PINK := Color("#f0b2a8")
const WHITE := Color("#f6f3ec")

## How tall each animal is, for its name label.
const HEIGHT := {
	"dog": 0.85, "cat": 0.6, "chicken": 0.65, "duck": 0.5, "sheep": 0.9,
	"cow": 1.5, "rabbit": 0.6, "fox": 0.75, "owl": 0.9,
}


## An animal. `title` adds a name label, `solid` a collider so students
## can't walk through it (always on for big animals).
static func make(kind: String, color: Color, title := "", solid := false) -> Node3D:
	var animal: Node3D = Animal.new()
	animal.kind = kind
	var body := Node3D.new()
	body.name = "Body"
	animal.add_child(body)
	var head := Node3D.new()
	head.name = "Head"
	body.add_child(head)
	var tail := Node3D.new()
	tail.name = "Tail"
	body.add_child(tail)
	match kind:
		"dog", "fox":
			var fox := kind == "fox"
			_lying(body, 0.15 if fox else 0.17, 0.8, Vector3(0, 0.45, 0), color)
			if fox:
				Builder.sphere(body, 0.13, Vector3(0, 0.42, 0.25), WHITE).scale = Vector3(0.9, 1.0, 0.8)
			head.position = Vector3(0, 0.68, 0.36)
			Builder.sphere(head, 0.15, Vector3.ZERO, color)
			Builder.box(head, Vector3(0.12, 0.09, 0.18), Vector3(0, -0.04, 0.16), WHITE if fox else color.lightened(0.15))
			Builder.sphere(head, 0.035, Vector3(0, -0.01, 0.25), EYE)
			_eyes(head, 0.065, Vector3(0, 0.05, 0.12))
			for x in [-1.0, 1.0]:
				if fox:
					var ear := Builder.cylinder(head, 0.0, 0.06, 0.14, Vector3(x * 0.08, 0.17, 0.0), color.darkened(0.1))
					ear.rotation.z = -x * 0.25
				else:
					var ear := Builder.box(head, Vector3(0.06, 0.16, 0.1), Vector3(x * 0.13, 0.02, -0.02), color.darkened(0.3))
					ear.rotation.z = x * 0.35
			_legs(body, 0.045, 0.32, 0.1, 0.25, color.darkened(0.15))
			tail.position = Vector3(0, 0.52, -0.38)
			if fox:
				var brush := Builder.sphere(tail, 0.1, Vector3(0, 0.0, -0.17), color)
				brush.scale = Vector3(0.9, 0.9, 2.0)
				Builder.sphere(tail, 0.07, Vector3(0, -0.02, -0.36), WHITE)
				tail.rotation.x = 0.35
			else:
				Builder.cylinder(tail, 0.02, 0.035, 0.32, Vector3(0, 0.14, -0.05), color).rotation.x = -0.6
		"cat":
			_lying(body, 0.11, 0.55, Vector3(0, 0.3, 0), color)
			head.position = Vector3(0, 0.45, 0.26)
			Builder.sphere(head, 0.12, Vector3.ZERO, color)
			_eyes(head, 0.05, Vector3(0, 0.03, 0.1), Color("#3c6e2f"))
			Builder.sphere(head, 0.02, Vector3(0, -0.02, 0.12), PINK)
			for x in [-1.0, 1.0]:
				var ear := Builder.cylinder(head, 0.0, 0.045, 0.1, Vector3(x * 0.06, 0.12, 0.0), color.darkened(0.1))
				ear.rotation.z = -x * 0.2
			_legs(body, 0.03, 0.2, 0.06, 0.16, color.darkened(0.1))
			tail.position = Vector3(0, 0.33, -0.26)
			Builder.cylinder(tail, 0.02, 0.025, 0.36, Vector3(0, 0.17, -0.03), color).rotation.x = -0.25
		"chicken":
			var hen := Builder.sphere(body, 0.17, Vector3(0, 0.3, 0), color)
			hen.scale = Vector3(0.9, 0.9, 1.15)
			Builder.box(body, Vector3(0.12, 0.16, 0.08), Vector3(0, 0.4, -0.17), color.darkened(0.08)).rotation.x = -0.5
			head.position = Vector3(0, 0.47, 0.11)
			Builder.sphere(head, 0.09, Vector3.ZERO, color)
			Builder.cylinder(head, 0.0, 0.03, 0.07, Vector3(0, -0.01, 0.1), BEAK).rotation.x = PI / 2.0
			Builder.box(head, Vector3(0.025, 0.07, 0.09), Vector3(0, 0.09, 0.0), Color("#d63c2f"))
			Builder.sphere(head, 0.025, Vector3(0, -0.06, 0.07), Color("#d63c2f"))
			_eyes(head, 0.05, Vector3(0, 0.02, 0.06))
			for x in [-0.05, 0.05]:
				Builder.cylinder(body, 0.012, 0.012, 0.16, Vector3(x, 0.08, 0.0), BEAK)
		"duck":
			var duck := Builder.sphere(body, 0.18, Vector3(0, 0.2, 0), color)
			duck.scale = Vector3(0.85, 0.7, 1.25)
			Builder.box(body, Vector3(0.1, 0.06, 0.1), Vector3(0, 0.27, -0.22), color.darkened(0.08)).rotation.x = -0.6
			head.position = Vector3(0, 0.36, 0.15)
			Builder.sphere(head, 0.095, Vector3.ZERO, color)
			Builder.box(head, Vector3(0.08, 0.03, 0.1), Vector3(0, -0.03, 0.11), BEAK)
			_eyes(head, 0.055, Vector3(0, 0.03, 0.06))
			for x in [-0.06, 0.06]:
				Builder.cylinder(body, 0.012, 0.012, 0.08, Vector3(x, 0.04, 0.0), BEAK)
		"sheep":
			for puff in [Vector3(0, 0.62, 0.2), Vector3(0, 0.62, -0.2), Vector3(0.1, 0.7, 0), Vector3(-0.1, 0.7, 0), Vector3(0, 0.55, 0)]:
				Builder.sphere(body, 0.27, puff, color)
			var face := Color("#3a3330")
			head.position = Vector3(0, 0.66, 0.44)
			Builder.sphere(head, 0.13, Vector3.ZERO, face).scale = Vector3(0.8, 1.0, 1.2)
			Builder.sphere(head, 0.1, Vector3(0, 0.1, -0.04), color)
			_eyes(head, 0.07, Vector3(0, 0.03, 0.1), Color.WHITE)
			for x in [-1.0, 1.0]:
				Builder.box(head, Vector3(0.12, 0.04, 0.06), Vector3(x * 0.14, 0.02, -0.02), face).rotation.z = x * 0.3
			_legs(body, 0.045, 0.42, 0.13, 0.2, face)
			tail.position = Vector3(0, 0.6, -0.45)
			Builder.sphere(tail, 0.08, Vector3.ZERO, color)
		"cow":
			Builder.box(body, Vector3(0.7, 0.7, 1.4), Vector3(0, 0.95, 0), color)
			var spot := Color("#2b2b2b")
			for at in [Vector3(1, 1.05, 0.25), Vector3(1, 0.9, -0.35), Vector3(-1, 1.0, 0.0), Vector3(-1, 1.1, -0.45)]:
				Builder.box(body, Vector3(0.02, 0.2, 0.24), Vector3(at.x * 0.355, at.y, at.z), spot)
			Builder.box(body, Vector3(0.22, 0.02, 0.28), Vector3(0, 1.305, -0.1), spot)
			Builder.sphere(body, 0.13, Vector3(0, 0.58, -0.3), PINK)
			head.position = Vector3(0, 1.15, 0.82)
			Builder.box(head, Vector3(0.4, 0.4, 0.42), Vector3.ZERO, color)
			Builder.box(head, Vector3(0.36, 0.2, 0.14), Vector3(0, -0.1, 0.26), PINK)
			_eyes(head, 0.12, Vector3(0, 0.08, 0.21))
			for x in [-1.0, 1.0]:
				Builder.cylinder(head, 0.015, 0.03, 0.16, Vector3(x * 0.15, 0.26, -0.05), WHITE).rotation.z = -x * 0.4
				Builder.box(head, Vector3(0.16, 0.06, 0.1), Vector3(x * 0.26, 0.1, -0.08), color.darkened(0.15))
			for x in [-0.24, 0.24]:
				for z in [-0.5, 0.5]:
					Builder.box(body, Vector3(0.14, 0.62, 0.14), Vector3(x, 0.31, z), color.darkened(0.1))
			tail.position = Vector3(0, 1.22, -0.71)
			Builder.cylinder(tail, 0.015, 0.02, 0.6, Vector3(0, -0.3, -0.02), color.darkened(0.2))
			Builder.sphere(tail, 0.05, Vector3(0, -0.6, -0.02), spot)
		"rabbit":
			var bun := Builder.sphere(body, 0.15, Vector3(0, 0.15, 0), color)
			bun.scale = Vector3(0.9, 0.85, 1.15)
			head.position = Vector3(0, 0.3, 0.13)
			Builder.sphere(head, 0.1, Vector3.ZERO, color)
			_eyes(head, 0.05, Vector3(0, 0.02, 0.08))
			Builder.sphere(head, 0.018, Vector3(0, -0.01, 0.1), PINK)
			for x in [-1.0, 1.0]:
				var ear := Builder.box(head, Vector3(0.045, 0.2, 0.025), Vector3(x * 0.04, 0.17, -0.02), color)
				ear.rotation.z = -x * 0.15
				ear.rotation.x = -0.15
			tail.position = Vector3(0, 0.17, -0.17)
			Builder.sphere(tail, 0.05, Vector3.ZERO, WHITE)
		"owl":
			var owl := Builder.cylinder(body, 0.17, 0.2, 0.5, Vector3(0, 0.33, 0), color)
			owl.scale.z = 0.85
			Builder.sphere(body, 0.16, Vector3(0, 0.3, 0.07), color.lightened(0.3)).scale = Vector3(0.9, 1.3, 0.6)
			for x in [-1.0, 1.0]:
				Builder.box(body, Vector3(0.05, 0.36, 0.26), Vector3(x * 0.2, 0.36, -0.02), color.darkened(0.2)).rotation.z = x * 0.08
				Builder.cylinder(body, 0.02, 0.02, 0.08, Vector3(x * 0.07, 0.04, 0.05), BEAK)
			head.position = Vector3(0, 0.7, 0)
			Builder.sphere(head, 0.17, Vector3.ZERO, color)
			for x in [-1.0, 1.0]:
				var disc := Builder.cylinder(head, 0.065, 0.065, 0.02, Vector3(x * 0.07, 0.02, 0.15), WHITE)
				disc.rotation.x = PI / 2.0
				Builder.sphere(head, 0.035, Vector3(x * 0.07, 0.02, 0.165), EYE)
				var tuft := Builder.cylinder(head, 0.0, 0.04, 0.1, Vector3(x * 0.1, 0.16, 0.0), color.darkened(0.2))
				tuft.rotation.z = -x * 0.35
			Builder.cylinder(head, 0.0, 0.025, 0.06, Vector3(0, -0.04, 0.17), BEAK).rotation.x = PI / 2.0
		_:
			Builder.sphere(body, 0.25, Vector3(0, 0.25, 0), color)
	if title != "":
		Builder.label(animal, title, HEIGHT.get(kind, 1.0) + 0.45, 40)
	if solid or kind in ["cow", "sheep"]:
		var collider := AnimatableBody3D.new()
		# Like walls: students bump into it, but the camera doesn't snap in
		# when an animal walks behind them.
		collider.collision_layer = Builder.WALL_LAYER
		var size := Vector3(0.8, 1.4, 1.9) if kind == "cow" else Vector3(0.5, HEIGHT.get(kind, 0.6), 0.7)
		Builder._box_shape(collider, size, Vector3(0, size.y / 2.0, 0))
		animal.add_child(collider)
		animal.collider = collider
	return animal


## A capsule lying along z, for four-legged bodies.
static func _lying(parent: Node3D, radius: float, length: float, at: Vector3, color: Color) -> void:
	var capsule := CapsuleMesh.new()
	capsule.radius = radius
	capsule.height = length
	var mesh := Builder._add_mesh(parent, capsule, at, color)
	mesh.rotation.x = PI / 2.0


static func _legs(parent: Node3D, radius: float, height: float, x: float, z: float, color: Color) -> void:
	for side in [-1.0, 1.0]:
		for end in [-1.0, 1.0]:
			Builder.cylinder(parent, radius, radius, height, Vector3(side * x, height / 2.0, end * z), color)


static func _eyes(head: Node3D, apart: float, at: Vector3, color := EYE) -> void:
	for x in [-1.0, 1.0]:
		Builder.sphere(head, 0.022, at + Vector3(x * apart, 0, 0), color)
