extends RefCounted
## Helpers for story data.
##
## A story is a plain Dictionary that round-trips through JSON, so a teacher's
## file stays readable. `normalized()` fills in every field; the rest of the
## code assumes a normalized story.

const VERSION := 1

const PROP_KINDS: Array[String] = [
	"house", "tree", "pine", "rock", "fence", "lamp",
	"bench", "well", "crate", "table", "shelf", "plant",
	"pond", "river", "bridge", "fountain",
]

const PROP_COLORS := {
	"house": "#e9d8b8", "tree": "#5c9e4c", "pine": "#3f7d4a",
	"rock": "#9b9a92", "fence": "#a27a52", "lamp": "#3c3c46",
	"bench": "#8c5b3c", "well": "#a9a297", "crate": "#ba8b50",
	"table": "#8c5b3c", "shelf": "#7b4f33", "plant": "#50995b",
	"pond": "#3f8fcf", "river": "#3f8fcf", "bridge": "#8c5b3c", "fountain": "#b9b3a7",
}

## How far from a prop's centre (in metres) a click still selects it.
const PROP_RADIUS := {
	"house": 2.4, "well": 1.2, "fence": 1.5, "rock": 1.0,
	"table": 1.1, "shelf": 1.1, "bench": 0.9,
	"pond": 2.6, "river": 2.0, "bridge": 1.8, "fountain": 1.5,
}

## Outdoor weather: how cloudy the sky is, and whether it rains.
const WEATHERS: Array[String] = ["clear", "cloudy", "rain"]

## Students arriving through a door appear this far in front of it.
const DOOR_SPAWN_DISTANCE := 2.6

## The largest story a link may unpack to, so a bad link can't use up memory.
const MAX_CODE_BYTES := 4 * 1024 * 1024

static var _BASE64 := RegEx.create_from_string("^[A-Za-z0-9+/]+={0,2}$")

const CHOICE_FIELDS: Array[String] = ["requires", "unless", "gives", "takes", "sets", "ending"]


static func blank() -> Dictionary:
	return normalized({
		"title": "My story",
		"places": [{"id": "town_square", "name": "Town square"}],
		"start_place": "town_square",
		"start_pos": [0, 4],
	})


static func normalized(story: Dictionary) -> Dictionary:
	var s: Dictionary = story.duplicate(true)
	s["version"] = VERSION
	_text(s, "title", "Untitled story")
	_text(s, "intro")
	_text(s, "goal")
	s["places"] = _list(s.get("places"))
	s["characters"] = _list(s.get("characters"))
	s["items"] = _list(s.get("items"))
	for place in s["places"]:
		fill_place(place)
	for character in s["characters"]:
		fill_character(character)
	for item in s["items"]:
		fill_item(item)
	_text(s, "start_place")
	if find(s, "places", s["start_place"]).is_empty() and not s["places"].is_empty():
		s["start_place"] = s["places"][0]["id"]
	s["start_pos"] = _pos(s.get("start_pos"))
	return s


static func fill_place(place: Dictionary) -> void:
	_text(place, "id")
	_text(place, "name", "Unnamed place")
	place["indoor"] = bool(place.get("indoor", false))
	var default_size := 16.0 if place["indoor"] else 36.0
	place["size"] = clampf(float(place.get("size", default_size)), 8.0, 120.0)
	_text(place, "ground", "#c9a77c" if place["indoor"] else "#86b85e")
	_text(place, "sky", "#efe1c6" if place["indoor"] else "#8ec5ff")
	_text(place, "weather", "clear")
	if not WEATHERS.has(place["weather"]):
		place["weather"] = "clear"
	place["props"] = _list(place.get("props"))
	for prop in place["props"]:
		_text(prop, "kind", "tree")
		if not PROP_KINDS.has(prop["kind"]):
			prop["kind"] = "crate"
		prop["pos"] = _pos(prop.get("pos"))
		prop["rot"] = float(prop.get("rot", 0.0))
		_text(prop, "color", PROP_COLORS[prop["kind"]])
	place["doors"] = _list(place.get("doors"))
	for door in place["doors"]:
		door["pos"] = _pos(door.get("pos"))
		door["rot"] = float(door.get("rot", 0.0))
		_text(door, "to")
		_text(door, "label")


static func fill_character(character: Dictionary) -> void:
	_text(character, "id")
	_text(character, "name", "Someone")
	_text(character, "place")
	character["pos"] = _pos(character.get("pos"))
	character["rot"] = float(character.get("rot", 0.0))
	_text(character, "color", "#d9825b")
	character["lines"] = _list(character.get("lines"))
	var lines: Array = character["lines"]
	for i in lines.size():
		var line: Dictionary = lines[i]
		_text(line, "text")
		line["opens"] = bool(line.get("opens", i == 0))
		_text(line, "requires")
		_text(line, "unless")
		line["choices"] = _list(line.get("choices"))
		for choice in line["choices"]:
			_text(choice, "text", "...")
			choice["next"] = int(choice.get("next", -1))
			for key in CHOICE_FIELDS:
				_text(choice, key)


static func fill_item(item: Dictionary) -> void:
	_text(item, "id")
	_text(item, "name", "Thing")
	_text(item, "description")
	_text(item, "place")
	item["pos"] = _pos(item.get("pos"))
	_text(item, "color", "#f2c14e")


static func new_line(text := "Hello!") -> Dictionary:
	return {"text": text, "opens": true, "requires": "", "unless": "", "choices": [new_choice("Goodbye.")]}


static func new_choice(text := "...") -> Dictionary:
	var choice := {"text": text, "next": -1}
	for key in CHOICE_FIELDS:
		choice[key] = ""
	return choice


static func find(story: Dictionary, key: String, id: String) -> Dictionary:
	for entry in story.get(key, []):
		if entry.get("id", "") == id:
			return entry
	return {}


static func slug(text: String) -> String:
	var out := ""
	for ch in text.to_lower():
		if (ch >= "a" and ch <= "z") or (ch >= "0" and ch <= "9"):
			out += ch
		elif out != "" and not out.ends_with("_"):
			out += "_"
	out = out.trim_suffix("_")
	return out if out != "" else "thing"


## An ID for a new entry in `story[key]`, made from `name`.
static func new_id(story: Dictionary, key: String, name: String) -> String:
	var stem := slug(name)
	var id := stem
	var n := 2
	while not find(story, key, id).is_empty():
		id = "%s_%d" % [stem, n]
		n += 1
	return id


## Every fact a choice can set, sorted.
static func flags(story: Dictionary) -> Array[String]:
	var out: Array[String] = []
	for character in story["characters"]:
		for line in character["lines"]:
			for choice in line["choices"]:
				var flag: String = choice["sets"]
				if flag != "" and not out.has(flag):
					out.append(flag)
	out.sort()
	return out


## The ground direction a thing rotated by `rot` degrees faces, as (x, z).
static func forward(rot: float) -> Vector2:
	return Vector2(sin(deg_to_rad(rot)), cos(deg_to_rad(rot)))


## Where a student appears when walking from one place into another: in front
## of the door that leads back, else at the start point, else the middle.
static func arrival(story: Dictionary, from_id: String, to_id: String) -> Array:
	var place := find(story, "places", to_id)
	for door in place.get("doors", []):
		if door["to"] == from_id:
			var f := forward(door["rot"])
			return [door["pos"][0] + f.x * DOOR_SPAWN_DISTANCE, door["pos"][1] + f.y * DOOR_SPAWN_DISTANCE]
	if to_id == story["start_place"]:
		return story["start_pos"].duplicate()
	return [0.0, 0.0]


## Mistakes a teacher would want to hear about before students play.
static func problems(story: Dictionary) -> Array[String]:
	var out: Array[String] = []
	if story["places"].is_empty():
		out.append("The story has no places.")
		return out
	var place_ids := {}
	for place in story["places"]:
		place_ids[place["id"]] = true
	var item_ids := {}
	for item in story["items"]:
		item_ids[item["id"]] = true
		if item["place"] != "" and not place_ids.has(item["place"]):
			out.append("%s is in a place that no longer exists." % item["name"])
	var known := item_ids.duplicate()
	for flag in flags(story):
		known[flag] = true
	for place in story["places"]:
		for door in place["doors"]:
			if not place_ids.has(door["to"]):
				out.append("A door in %s doesn't lead anywhere." % place["name"])
			elif door["to"] == place["id"]:
				out.append("A door in %s leads back into itself." % place["name"])
	var has_ending := false
	for character in story["characters"]:
		var who: String = character["name"]
		if not place_ids.has(character["place"]):
			out.append("%s isn't in any place." % who)
		var lines: Array = character["lines"]
		if lines.is_empty():
			out.append("%s has nothing to say." % who)
			continue
		var opens := false
		for i in lines.size():
			var line: Dictionary = lines[i]
			var where := "%s, line %d" % [who, i + 1]
			if line["opens"]:
				opens = true
				for key in ["requires", "unless"]:
					if line[key] != "" and not known.has(line[key]):
						out.append("%s: nothing gives or sets \"%s\"." % [where, line[key]])
			for choice in line["choices"]:
				if choice["ending"] != "":
					has_ending = true
				var next: int = choice["next"]
				if next >= lines.size():
					out.append("%s: a choice goes to line %d, which doesn't exist." % [where, next + 1])
				for key in ["requires", "unless"]:
					if choice[key] != "" and not known.has(choice[key]):
						out.append("%s: nothing gives or sets \"%s\"." % [where, choice[key]])
				for key in ["gives", "takes"]:
					if choice[key] != "" and not item_ids.has(choice[key]):
						out.append("%s: \"%s\" isn't an item." % [where, choice[key]])
		if not opens:
			out.append("%s has no line that can start a conversation." % who)
	if not has_ending:
		out.append("No choice ends the story yet.")
	return out


static func load_file(path: String) -> Dictionary:
	if not FileAccess.file_exists(path):
		return {"error": "%s doesn't exist." % path}
	return parse(FileAccess.get_file_as_string(path), path.get_file())


## Reads a story from JSON text. Returns {"story": ...} or {"error": message}.
static func parse(text: String, file_name := "The file") -> Dictionary:
	var json := JSON.new()
	if json.parse(text) != OK:
		return {"error": "%s isn't valid JSON (line %d: %s)." % [file_name, json.get_error_line(), json.get_error_message()]}
	if not (json.data is Dictionary):
		return {"error": "%s isn't a story." % file_name}
	return {"story": normalized(json.data)}


## The story packed into a short URL-safe code, so a whole story fits in a
## link students can open without the teacher hosting any files.
static func to_code(story: Dictionary) -> String:
	var packed := JSON.stringify(story).to_utf8_buffer().compress(FileAccess.COMPRESSION_DEFLATE)
	return Marshalls.raw_to_base64(packed).replace("+", "-").replace("/", "_").trim_suffix("=").trim_suffix("=")


## Reads a story back from to_code(). Returns {"story": ...} or {"error": message}.
static func from_code(code: String) -> Dictionary:
	var b64 := code.strip_edges().replace("-", "+").replace("_", "/")
	b64 += "=".repeat((4 - b64.length() % 4) % 4)
	var packed := PackedByteArray()
	if _BASE64.search(b64):
		packed = Marshalls.base64_to_raw(b64)
	var bytes := PackedByteArray()
	if not packed.is_empty():
		bytes = packed.decompress_dynamic(MAX_CODE_BYTES, FileAccess.COMPRESSION_DEFLATE)
	if bytes.is_empty():
		return {"error": "This story link is broken or incomplete."}
	return parse(bytes.get_string_from_utf8(), "The story in this link")


## Writes the story and returns an error message, or "" on success.
static func save_file(story: Dictionary, path: String) -> String:
	DirAccess.make_dir_recursive_absolute(path.get_base_dir())
	var file := FileAccess.open(path, FileAccess.WRITE)
	if file == null:
		return "Couldn't write %s (%s)." % [path, error_string(FileAccess.get_open_error())]
	file.store_string(JSON.stringify(story, "\t") + "\n")
	file.close()
	return ""


static func _text(d: Dictionary, key: String, fallback := "") -> void:
	var value = d.get(key)
	d[key] = fallback if value == null else str(value)


static func _list(value) -> Array:
	var out := []
	if value is Array:
		for entry in value:
			if entry is Dictionary:
				out.append(entry)
	return out


static func _pos(value) -> Array:
	if value is Array and value.size() >= 2:
		return [float(value[0]), float(value[1])]
	return [0.0, 0.0]
