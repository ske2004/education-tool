extends RefCounted
## The app's look: warm paper panels, rounded buttons and dark ink.

const INK := Color("#2e2a25")
const MUTED := Color("#7a7064")
const PAPER := Color("#fbf7f0")
const SIDEBAR := Color("#f4ede2")
const SAND := Color("#efe6d8")
const SAND_DARK := Color("#e2d5c0")
const LINE := Color("#d9cdbb")
const ACCENT := Color("#d9694a")
const ACCENT_LIGHT := Color("#f6d9cf")
const WARNING := Color("#b5462f")
const GOOD := Color("#3d8b4f")

const TEXT_TYPES := [
	"Label", "Button", "OptionButton", "MenuButton", "CheckBox", "CheckButton",
	"LineEdit", "TextEdit", "PopupMenu", "Tree", "ItemList", "LinkButton",
	"TooltipLabel", "FileDialog", "AcceptDialog", "ConfirmationDialog",
]
const BUTTON_TYPES := ["Button", "OptionButton", "MenuButton"]

static var _shared: Theme


## The theme, made once.
static func shared() -> Theme:
	if _shared == null:
		_shared = make()
	return _shared


## Themes every Control or Window under `node` that doesn't inherit a theme
## from a parent Control (those under a CanvasLayer or a plain Node).
static func apply(node: Node) -> void:
	var parent := node.get_parent()
	if (node is Control and not parent is Control) or node is Window:
		node.set("theme", shared())
	for child in node.get_children():
		apply(child)


static func make() -> Theme:
	var t := Theme.new()
	t.default_font_size = 17
	for type in TEXT_TYPES:
		for item in ["font_color", "font_hover_color", "font_pressed_color", "font_focus_color", "font_hover_pressed_color"]:
			t.set_color(item, type, INK)
		t.set_color("font_disabled_color", type, MUTED)
		t.set_color("font_placeholder_color", type, MUTED)
	t.set_color("default_color", "RichTextLabel", INK)
	t.set_color("title_color", "Window", INK)
	t.set_color("caret_color", "LineEdit", INK)
	t.set_color("caret_color", "TextEdit", INK)
	t.set_color("font_selected_color", "LineEdit", INK)
	t.set_color("selection_color", "LineEdit", ACCENT_LIGHT)
	t.set_color("selection_color", "TextEdit", ACCENT_LIGHT)

	for type in BUTTON_TYPES:
		t.set_stylebox("normal", type, box(SAND, 8, LINE))
		t.set_stylebox("hover", type, box(SAND_DARK, 8, LINE))
		t.set_stylebox("pressed", type, box(ACCENT_LIGHT, 8, ACCENT))
		t.set_stylebox("hover_pressed", type, box(ACCENT_LIGHT, 8, ACCENT))
		t.set_stylebox("disabled", type, box(SAND, 8, LINE))
		t.set_stylebox("focus", type, StyleBoxEmpty.new())
	for state in ["normal", "hover", "pressed", "hover_pressed", "focus", "disabled"]:
		t.set_stylebox(state, "CheckBox", StyleBoxEmpty.new())
	for type in ["LineEdit", "TextEdit"]:
		t.set_stylebox("normal", type, box(Color.WHITE, 6, LINE, 8))
		t.set_stylebox("focus", type, box(Color.WHITE, 6, ACCENT, 8))
		t.set_stylebox("read_only", type, box(SAND, 6, LINE, 8))

	var panel := box(PAPER, 12, LINE, 14)
	panel.shadow_color = Color(0, 0, 0, 0.12)
	panel.shadow_size = 8
	for type in ["PanelContainer", "Panel", "AcceptDialog", "ConfirmationDialog", "FileDialog", "TooltipPanel"]:
		t.set_stylebox("panel", type, panel)
	t.set_stylebox("embedded_border", "Window", box(PAPER, 10, LINE, 0))
	t.set_stylebox("panel", "PopupMenu", box(PAPER, 8, LINE, 6))
	t.set_stylebox("hover", "PopupMenu", box(SAND_DARK, 6))
	for type in ["Tree", "ItemList"]:
		t.set_stylebox("panel", type, box(Color.WHITE, 6, LINE, 4))

	_variation(t, "Card", "PanelContainer")
	var card := box(PAPER, 16, LINE, 20)
	card.shadow_color = Color(0, 0, 0, 0.18)
	card.shadow_size = 14
	t.set_stylebox("panel", "Card", card)
	_variation(t, "Bar", "PanelContainer")
	var bar := box(PAPER, 0, Color.TRANSPARENT, 10)
	bar.border_color = LINE
	bar.border_width_bottom = 1
	t.set_stylebox("panel", "Bar", bar)
	_variation(t, "Sidebar", "PanelContainer")
	t.set_stylebox("panel", "Sidebar", box(SIDEBAR, 0, Color.TRANSPARENT, 12))
	_variation(t, "Chip", "PanelContainer")
	t.set_stylebox("panel", "Chip", box(PAPER, 16, LINE, 8))
	_variation(t, "LinePanel", "PanelContainer")
	t.set_stylebox("panel", "LinePanel", box(Color.WHITE, 10, LINE, 10))
	_variation(t, "ChoicePanel", "PanelContainer")
	t.set_stylebox("panel", "ChoicePanel", box(PAPER, 8, LINE, 8))

	_variation(t, "AccentButton", "Button")
	t.set_stylebox("normal", "AccentButton", box(ACCENT, 8))
	t.set_stylebox("hover", "AccentButton", box(ACCENT.lightened(0.1), 8))
	t.set_stylebox("pressed", "AccentButton", box(ACCENT.darkened(0.1), 8))
	for item in ["font_color", "font_hover_color", "font_pressed_color", "font_focus_color"]:
		t.set_color(item, "AccentButton", Color.WHITE)
	_variation(t, "ListButton", "Button")
	t.set_stylebox("normal", "ListButton", box(Color.TRANSPARENT, 6))
	t.set_stylebox("hover", "ListButton", box(SAND_DARK, 6))
	t.set_stylebox("pressed", "ListButton", box(ACCENT_LIGHT, 6))
	t.set_stylebox("hover_pressed", "ListButton", box(ACCENT_LIGHT, 6))
	_variation(t, "SmallButton", "Button")
	t.set_font_size("font_size", "SmallButton", 14)
	t.set_stylebox("normal", "SmallButton", box(SAND, 6, LINE, 4))
	t.set_stylebox("hover", "SmallButton", box(SAND_DARK, 6, LINE, 4))
	t.set_stylebox("pressed", "SmallButton", box(ACCENT_LIGHT, 6, ACCENT, 4))

	_label(t, "TitleLabel", 44, INK)
	_label(t, "HeaderLabel", 24, INK)
	_label(t, "SectionLabel", 14, MUTED)
	_label(t, "FieldLabel", 14, MUTED)
	_label(t, "MutedLabel", 15, MUTED)
	_label(t, "WarningLabel", 15, WARNING)
	_label(t, "GoodLabel", 15, GOOD)
	return t


static func box(bg: Color, radius: int, border := Color.TRANSPARENT, margin := 10) -> StyleBoxFlat:
	var s := StyleBoxFlat.new()
	s.bg_color = bg
	s.set_corner_radius_all(radius)
	if border.a > 0.0:
		s.border_color = border
		s.set_border_width_all(1)
	s.content_margin_left = margin + 4
	s.content_margin_right = margin + 4
	s.content_margin_top = margin
	s.content_margin_bottom = margin
	return s


static func _variation(t: Theme, name: String, base: String) -> void:
	t.set_type_variation(name, base)


static func _label(t: Theme, name: String, size: int, color: Color) -> void:
	_variation(t, name, "Label")
	t.set_font_size("font_size", name, size)
	t.set_color("font_color", name, color)
