extends Node3D

@onready var tex_rect = get_node("TextureRect")
func _ready():
	RetroHost.load_core("dosbox_pure_libretro")

func _process(_delta):
	# Run one iteration of the core
	RetroHost.run()

	# Retrieve the framebuffer image
	var frame_buffer = RetroHost.get_frame_buffer()
	if(!frame_buffer):
		return
	
	# Fuckery to display the image
	var img_tex = ImageTexture.create_from_image(frame_buffer)
	$MeshInstance3D.get_surface_override_material(0).albedo_texture = img_tex
	return
	
func _input(event):
	# Forward input events to the frontend  (duh)
	RetroHost.forward_input(event)
