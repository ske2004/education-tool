extends Node3D
## Moves slowly along x and wraps around, for clouds.

var speed := 0.6
## Wraps back to -limit after passing +limit.
var limit := 60.0


func _process(delta: float) -> void:
	position.x += speed * delta
	if position.x > limit:
		position.x -= limit * 2.0
