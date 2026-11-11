extends Node
## Opening and saving story files, on the desktop and in a browser.
##
## On the desktop this uses the system's file dialogs. In a browser it uploads
## a file the teacher picks and downloads the story when they save, keeps a
## draft of the story being edited in the browser, and makes story links.

signal opened(story: Dictionary, file_path: String)
signal saved(file_path: String)
signal failed(message: String)

const StoryData = preload("res://scripts/story.gd")
const UITheme = preload("res://scripts/ui_theme.gd")

const DRAFT := "user://draft.json"
const LINK_KEY := "story="

## Opens a file picker in the page and passes the chosen file's name and text
## to a Godot callback.
const UPLOAD_JS := """
window.educoreUpload = function (done) {
	const input = document.createElement('input');
	input.type = 'file';
	input.accept = '.json,application/json';
	input.onchange = function () {
		const file = input.files[0];
		if (file) file.text().then(function (text) { done(file.name, text); });
	};
	input.click();
};
"""

var _open_dialog: FileDialog
var _save_dialog: FileDialog
var _upload_callback: JavaScriptObject
var _story_to_save: Dictionary


static func on_web() -> bool:
	return OS.has_feature("web")


func _ready() -> void:
	if on_web():
		JavaScriptBridge.eval(UPLOAD_JS, true)
		_upload_callback = JavaScriptBridge.create_callback(_on_upload)
		return
	_open_dialog = _dialog(FileDialog.FILE_MODE_OPEN_FILE)
	_open_dialog.file_selected.connect(_open_path)
	_save_dialog = _dialog(FileDialog.FILE_MODE_SAVE_FILE)
	_save_dialog.file_selected.connect(func(file_path): save(_story_to_save, file_path))


func _dialog(mode: FileDialog.FileMode) -> FileDialog:
	var dialog := FileDialog.new()
	dialog.file_mode = mode
	dialog.access = FileDialog.ACCESS_FILESYSTEM
	dialog.filters = PackedStringArray(["*.json ; Story files"])
	dialog.use_native_dialog = true
	dialog.current_dir = OS.get_system_dir(OS.SYSTEM_DIR_DOCUMENTS)
	add_child(dialog)
	UITheme.apply(dialog)
	return dialog


## Asks for a story file; emits `opened` or `failed`.
func ask_open() -> void:
	if on_web():
		JavaScriptBridge.get_interface("window").educoreUpload(_upload_callback)
	else:
		_open_dialog.popup_centered_ratio(0.6)


func _open_path(file_path: String) -> void:
	_report(StoryData.load_file(file_path), file_path)


func _on_upload(args: Array) -> void:
	_report(StoryData.parse(str(args[1]), str(args[0])), str(args[0]))


func _report(result: Dictionary, file_path: String) -> void:
	if result.has("error"):
		failed.emit(result["error"])
	else:
		opened.emit(result["story"], file_path)


## Saves to `file_path`, or downloads the story in a browser. Emits `saved`
## or `failed`.
func save(story: Dictionary, file_path: String) -> void:
	if on_web():
		var file_name := StoryData.slug(story["title"]) + ".json"
		var text := JSON.stringify(story, "\t") + "\n"
		JavaScriptBridge.download_buffer(text.to_utf8_buffer(), file_name, "application/json")
		saved.emit(file_name)
		return
	var error := StoryData.save_file(story, file_path)
	if error != "":
		failed.emit(error)
	else:
		saved.emit(file_path)


## Asks where to save; in a browser there is nothing to ask, so it downloads.
func ask_save(story: Dictionary) -> void:
	_story_to_save = story
	if on_web():
		save(story, "")
		return
	_save_dialog.current_file = StoryData.slug(story["title"]) + ".json"
	_save_dialog.popup_centered_ratio(0.6)


# Drafts and links (browser only) ----------------------------------------------

static func save_draft(story: Dictionary) -> void:
	StoryData.save_file(story, DRAFT)


static func load_draft() -> Dictionary:
	if not on_web() or not FileAccess.file_exists(DRAFT):
		return {}
	return StoryData.load_file(DRAFT).get("story", {})


## A link to this page that plays `story`, or "" outside a browser.
static func link_for(story: Dictionary) -> String:
	if not on_web():
		return ""
	var page := str(JavaScriptBridge.eval("window.location.origin + window.location.pathname"))
	return page + "#" + LINK_KEY + StoryData.to_code(story)


## The story in this page's link: {} if there is none, else {"story": ...}
## or {"error": message}.
static func story_in_link() -> Dictionary:
	if not on_web():
		return {}
	var hash_text := str(JavaScriptBridge.eval("window.location.hash"))
	var at := hash_text.find(LINK_KEY)
	if at < 0:
		return {}
	return StoryData.from_code(hash_text.substr(at + LINK_KEY.length()))
