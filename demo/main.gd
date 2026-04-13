extends VBoxContainer

@onready var core_selector: OptionButton = %CoreSelector
@onready var load_game_button: Button = %LoadGameButton
@onready var settings_button: Button = %SettingsButton
@onready var game_display: TextureRect = %GameDisplay
@onready var settings_panel: VBoxContainer = %SettingsPanel
@onready var settings_vbox: VBoxContainer = %SettingsVBox
@onready var shader_selector: OptionButton = %ShaderSelector
@onready var file_dialog: FileDialog = %FileDialog

var crt_shader: Shader = preload("res://shaders/crt.gdshader")
var lcd_shader: Shader = preload("res://shaders/lcd.gdshader")

var core_loaded := false
var core_names: Array[String] = []
# Maps file extension (e.g. "bin") -> Array of core names that support it
var ext_to_core: Dictionary = {}
# Maps core name -> library_name from retro_get_system_info (e.g. "genesis_plus_gx" -> "Genesis Plus GX")
var core_library_names: Dictionary = {}
# If user manually picks a core, use it instead of auto-detection
var manual_core_override := false
var _scan_thread: Thread

func _ready():
	_scan_cores()
	core_selector.item_selected.connect(_on_core_selected)
	load_game_button.pressed.connect(_on_load_game_pressed)
	settings_button.pressed.connect(_on_settings_pressed)
	file_dialog.file_selected.connect(_on_file_selected)
	shader_selector.item_selected.connect(_on_shader_selected)
	settings_panel.visible = false
	# Enable Load Game from the start for ROM-first workflow
	load_game_button.disabled = false

func _scan_cores():
	var cores_path := "res://libretro-cores/"
	var dir := DirAccess.open(cores_path)
	if not dir:
		printerr("Could not open libretro-cores/ directory")
		return

	core_selector.clear()
	core_selector.add_item("-- Select Core --")

	core_names = []
	dir.list_dir_begin()
	var file_name := dir.get_next()
	while file_name != "":
		if not dir.current_is_dir() and _is_core_library(file_name):
			core_names.append(file_name.get_basename())
		file_name = dir.get_next()
	dir.list_dir_end()

	core_names.sort()
	for core_name in core_names:
		core_selector.add_item(core_name)

	# Build extension -> core mapping (cached to disk, scanned in background)
	_load_or_build_ext_to_core_map()

const EXT_CACHE_PATH := "user://ext_to_core_cache.json"

func _load_or_build_ext_to_core_map():
	ext_to_core.clear()
	# Try loading from cache on the main thread (fast)
	if FileAccess.file_exists(EXT_CACHE_PATH):
		var file := FileAccess.open(EXT_CACHE_PATH, FileAccess.READ)
		if file:
			var parsed = JSON.parse_string(file.get_as_text())
			file.close()
			if parsed is Dictionary:
				var cached_cores: Array = parsed.get("core_names", [])
				if cached_cores == Array(core_names):
					ext_to_core = parsed.get("ext_to_core", {})
					core_library_names = parsed.get("core_library_names", {})
					print("[main] Loaded ext_to_core cache (%d extensions)" % ext_to_core.size())
					return
	# Cache miss — scan in background thread
	print("[main] Building ext_to_core map in background...")
	_scan_thread = Thread.new()
	_scan_thread.start(_scan_core_extensions_thread)

func _scan_core_extensions_thread():
	var ext_map: Dictionary = {}
	var lib_names: Dictionary = {}
	for core_name in core_names:
		var info: Dictionary = RetroHost.get_core_info(core_name)
		if info.is_empty():
			continue
		lib_names[core_name] = info.get("library_name", "")
		var extensions_str: String = info.get("valid_extensions", "")
		if extensions_str.is_empty():
			continue
		for ext in extensions_str.split("|"):
			ext = ext.strip_edges().to_lower()
			if ext.is_empty():
				continue
			if not ext_map.has(ext):
				ext_map[ext] = []
			ext_map[ext].append(core_name)
	call_deferred("_on_scan_complete", ext_map, lib_names)

func _on_scan_complete(ext_map: Dictionary, lib_names: Dictionary):
	_scan_thread.wait_to_finish()
	_scan_thread = null
	ext_to_core = ext_map
	core_library_names = lib_names
	# Write cache
	var cache := {
		"core_names": Array(core_names),
		"ext_to_core": ext_to_core,
		"core_library_names": core_library_names,
	}
	var file := FileAccess.open(EXT_CACHE_PATH, FileAccess.WRITE)
	if file:
		file.store_string(JSON.stringify(cache))
		file.close()
	print("[main] Cached ext_to_core map (%d extensions)" % ext_to_core.size())

func _is_core_library(file_name: String) -> bool:
	return file_name.ends_with(".dylib") or file_name.ends_with(".so") or file_name.ends_with(".dll")

func _on_core_selected(index: int):
	if index == 0:
		manual_core_override = false
		return
	var core_name := core_selector.get_item_text(index)
	core_loaded = RetroHost.load_core(core_name)
	if core_loaded:
		manual_core_override = true
		_build_settings_panel()

func _exit_tree():
	if _scan_thread and _scan_thread.is_started():
		_scan_thread.wait_to_finish()

func _on_load_game_pressed():
	file_dialog.popup_centered_ratio(0.7)

func _on_settings_pressed():
	settings_panel.visible = not settings_panel.visible

func _on_file_selected(path: String):
	# Auto-select core based on ROM if no manual override
	if not manual_core_override or not core_loaded:
		var auto_core := _resolve_core_for_rom(path)
		if auto_core.is_empty():
			printerr("[main] No core found for '%s'" % path.get_file())
			return
		print("[main] Auto-selecting core '%s' for '%s'" % [auto_core, path.get_file()])
		core_loaded = RetroHost.load_core(auto_core)
		if core_loaded:
			for i in range(core_selector.item_count):
				if core_selector.get_item_text(i) == auto_core:
					core_selector.select(i)
					break
			_build_settings_panel()
		else:
			printerr("[main] Failed to auto-load core '%s'" % auto_core)
			return

	if core_loaded:
		RetroHost.load_game(path)
		_release_ui_focus()

## Resolve which core to use for a ROM file.
## Uses extension first; if ambiguous, reads magic bytes to identify the system.
func _resolve_core_for_rom(path: String) -> String:
	var ext := path.get_extension().to_lower()
	if not ext_to_core.has(ext):
		return ""
	var candidates: Array = ext_to_core[ext]
	if candidates.size() == 1:
		return candidates[0]
	# Multiple cores claim this extension — use header detection to disambiguate
	var system := _identify_system_from_header(path)
	if not system.is_empty():
		for core_name in candidates:
			if _core_matches_system(core_name, system):
				return core_name
	# Fallback: first candidate
	return candidates[0]

## Read magic bytes from a ROM file to identify the system.
## Returns a system tag like "genesis", "sms", "saturn", "psx", "n64", "nes", "snes", "gb", "gba".
func _identify_system_from_header(path: String) -> String:
	var file := FileAccess.open(path, FileAccess.READ)
	if not file:
		return ""
	var header := file.get_buffer(0x300)
	file.close()
	var size := header.size()

	# NES: "NES\x1a" at offset 0
	if size >= 4 and header[0] == 0x4E and header[1] == 0x45 and header[2] == 0x53 and header[3] == 0x1A:
		return "nes"

	# Sega Genesis/Mega Drive: "SEGA" at offset 0x100
	if size >= 0x104:
		var sega_str := header.slice(0x100, 0x104).get_string_from_ascii()
		if sega_str == "SEGA":
			# Check for 32X: " 32X" at 0x104
			if size >= 0x108:
				var sub := header.slice(0x104, 0x108).get_string_from_ascii()
				if sub == " 32X":
					return "32x"
			return "genesis"

	# Sega Saturn: "SEGA SEGASATURN" near offset 0x00 (disc image)
	if size >= 0x10:
		var sat_str := header.slice(0x00, 0x10).get_string_from_ascii()
		if "SEGASATURN" in sat_str:
			return "saturn"

	# N64: big-endian 0x80371240 at offset 0 (or byte-swapped variants)
	if size >= 4:
		if header[0] == 0x80 and header[1] == 0x37 and header[2] == 0x12 and header[3] == 0x40:
			return "n64"
		if header[0] == 0x37 and header[1] == 0x80 and header[2] == 0x40 and header[3] == 0x12:
			return "n64"
		if header[0] == 0x40 and header[1] == 0x12 and header[2] == 0x37 and header[3] == 0x80:
			return "n64"

	# Game Boy: Nintendo logo at 0xCE-0xD0 area, or check 0x104 for logo start
	if size >= 0x150:
		# GB/GBC cartridge header: 0x104..0x133 is Nintendo logo
		if header[0x104] == 0xCE and header[0x105] == 0xED and header[0x106] == 0x66 and header[0x107] == 0x66:
			# Check GBC flag at 0x143
			if header[0x143] == 0xC0 or header[0x143] == 0x80:
				return "gbc"
			return "gb"

	# GBA: Nintendo logo at 0x04, ROM entry point at 0x00 is ARM branch
	if size >= 0xC0:
		if header[0x04] == 0x24 and header[0x05] == 0xFF and header[0x06] == 0xAE and header[0x07] == 0x51:
			return "gba"

	# PlayStation: CD-ROM sync pattern (00 FF*10 00) at offset 0 for raw .bin disc images
	if size >= 16:
		if header[0] == 0x00 and header[1] == 0xFF and header[2] == 0xFF and header[3] == 0xFF \
			and header[4] == 0xFF and header[5] == 0xFF and header[6] == 0xFF and header[7] == 0xFF \
			and header[8] == 0xFF and header[9] == 0xFF and header[10] == 0xFF and header[11] == 0x00:
			# It's a raw CD image — check for "PlayStation" string in the system area
			file = FileAccess.open(path, FileAccess.READ)
			if file:
				var sector := file.get_buffer(0x940)  # read first sector + a bit
				file.close()
				var sector_str := sector.get_string_from_ascii()
				if "PlayStation" in sector_str:
					return "psx"
				if "SEGADISC" in sector_str or "SEGASATURN" in sector_str:
					return "saturn"
			return "cd_unknown"

	# SNES: check for valid header at 0x7FC0 (LoROM) or 0xFFC0 (HiROM) — need larger read
	# Skip for now; .sfc/.smc are unambiguous extensions

	return ""

# Known system tags that each core handles, matched against library_name or core filename.
const SYSTEM_TO_CORE_HINTS := {
	"genesis": ["genesis_plus_gx", "genesis", "picodrive", "blastem"],
	"32x": ["picodrive"],
	"sms": ["genesis_plus_gx", "genesis", "picodrive", "smsplus"],
	"saturn": ["mednafen_saturn", "yabause", "kronos", "beetle_saturn", "beetle-saturn"],
	"psx": ["pcsx_rearmed", "beetle_psx", "beetle-psx", "mednafen_psx", "swanstation", "duckstation"],
	"n64": ["mupen64plus", "parallel_n64", "parallel-n64"],
	"nes": ["fceumm", "nestopia", "mesen", "quicknes"],
	"gb": ["gambatte", "mgba", "sameboy", "vbam", "gearboy"],
	"gbc": ["gambatte", "mgba", "sameboy", "vbam", "gearboy"],
	"gba": ["mgba", "vbam", "vba_next", "gpsp", "meteor"],
}

## Check if a core (by filename or library_name) matches a detected system tag.
func _core_matches_system(core_name: String, system: String) -> bool:
	if not SYSTEM_TO_CORE_HINTS.has(system):
		return false
	var hints: Array = SYSTEM_TO_CORE_HINTS[system]
	var cn := core_name.to_lower()
	var lib_name: String = core_library_names.get(core_name, "").to_lower()
	for hint in hints:
		if hint in cn or hint in lib_name:
			return true
	return false

func _process(_delta):
	RetroHost.run()
	var frame_buffer = RetroHost.get_frame_buffer()
	if frame_buffer:
		game_display.texture = ImageTexture.create_from_image(frame_buffer)

func _input(event):
	if event is InputEventMouseButton and event.pressed:
		_release_ui_focus()
	RetroHost.forward_input(event)

func _release_ui_focus():
	var focused := get_viewport().gui_get_focus_owner()
	if focused:
		focused.release_focus()

func _build_settings_panel():
	# Clear existing settings
	for child in settings_vbox.get_children():
		child.queue_free()

	var variables: Dictionary = RetroHost.get_core_variables()
	for key in variables:
		var value: String = variables[key]
		var options: PackedStringArray = RetroHost.get_core_variable_options(key)

		var hbox := HBoxContainer.new()
		var label := Label.new()
		label.text = key
		label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		label.clip_text = true
		hbox.add_child(label)

		if options.size() > 0:
			var option_btn := OptionButton.new()
			for i in range(options.size()):
				option_btn.add_item(options[i])
				if options[i] == value:
					option_btn.select(i)
			option_btn.item_selected.connect(_on_setting_changed.bind(key, option_btn))
			option_btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
			hbox.add_child(option_btn)
		else:
			var line_edit := LineEdit.new()
			line_edit.text = value
			line_edit.text_submitted.connect(_on_setting_text_changed.bind(key))
			line_edit.size_flags_horizontal = Control.SIZE_EXPAND_FILL
			hbox.add_child(line_edit)

		settings_vbox.add_child(hbox)

func _on_setting_changed(index: int, key: String, option_btn: OptionButton):
	var value := option_btn.get_item_text(index)
	RetroHost.set_core_variable(key, value)

func _on_setting_text_changed(value: String, key: String):
	RetroHost.set_core_variable(key, value)

func _on_shader_selected(index: int):
	match index:
		0: # None
			game_display.material = null
		1: # CRT
			var mat := ShaderMaterial.new()
			mat.shader = crt_shader
			game_display.material = mat
		2: # LCD
			var mat := ShaderMaterial.new()
			mat.shader = lcd_shader
			game_display.material = mat
