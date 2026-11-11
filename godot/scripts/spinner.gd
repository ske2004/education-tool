extends Node3D
## Spins and bobs an item's "Gem" child so it reads as something to pick up.

const HEIGHT := 1.15

var _t := randf() * TAU
var _gem: Node3D


func _ready() -> void:
	_gem = get_node_or_null("Gem")


func _process(delta: float) -> void:
	_t += delta
	if _gem:
		_gem.rotation.y += delta * 1.6
		_gem.position.y = HEIGHT + sin(_t * 2.2) * 0.08
