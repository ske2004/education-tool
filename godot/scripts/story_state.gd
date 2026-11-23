extends RefCounted
## A student's progress through a story: where they are, what they carry,
## which facts they've learned and whether the story has ended.

const StoryData = preload("res://scripts/story.gd")

var story: Dictionary
var place: String
var pos: Array
var inventory: Array[String] = []
var flags := {}
## Items taken out of the world, so they don't reappear.
var collected := {}
var ending := ""


func _init(s: Dictionary) -> void:
	story = s
	place = s["start_place"]
	pos = s["start_pos"].duplicate()


func has(fact: String) -> bool:
	return fact == "" or inventory.has(fact) or flags.has(fact)


## Whether a line or choice's "requires" and "unless" conditions hold.
func allows(entry: Dictionary) -> bool:
	var requires: String = entry.get("requires", "")
	var unless: String = entry.get("unless", "")
	return (requires == "" or has(requires)) and (unless == "" or not has(unless))


## The line a conversation starts at, or -1 when there's nothing to say.
func opening_line(character: Dictionary) -> int:
	var lines: Array = character["lines"]
	for i in lines.size():
		if lines[i]["opens"] and allows(lines[i]):
			return i
	return -1


## The choices a student can pick on a line, as [{index, text}].
func choices(character: Dictionary, line: int) -> Array:
	var out := []
	var options: Array = character["lines"][line]["choices"]
	for i in options.size():
		if allows(options[i]):
			out.append({"index": i, "text": options[i]["text"]})
	return out


## Applies a choice's effects and returns the next line, or -1 to stop talking.
func choose(character: Dictionary, line: int, index: int) -> int:
	var lines: Array = character["lines"]
	var choice: Dictionary = lines[line]["choices"][index]
	if choice["gives"] != "":
		_receive(choice["gives"])
	if choice["takes"] != "":
		inventory.erase(choice["takes"])
	if choice["sets"] != "":
		flags[choice["sets"]] = true
	if choice["ending"] != "":
		ending = choice["ending"]
	var next: int = choice["next"]
	return next if next >= 0 and next < lines.size() else -1


func pick_up(item_id: String) -> void:
	_receive(item_id)


func travel(to_place: String) -> void:
	pos = StoryData.arrival(story, place, to_place)
	place = to_place


func item(id: String) -> Dictionary:
	return StoryData.find(story, "items", id)


func _receive(item_id: String) -> void:
	collected[item_id] = true
	if not inventory.has(item_id):
		inventory.append(item_id)
