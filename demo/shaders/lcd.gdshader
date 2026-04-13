shader_type canvas_item;

uniform float grid_strength : hint_range(0.0, 1.0) = 0.25;
uniform float brightness_boost : hint_range(1.0, 2.0) = 1.3;

void fragment() {
	vec4 col = texture(TEXTURE, UV);
	vec2 tex_size = vec2(textureSize(TEXTURE, 0));
	vec2 pixel_pos = UV * tex_size;

	// Determine which subpixel column we're in (R, G, or B)
	int sub = int(mod(pixel_pos.x * 3.0, 3.0));
	vec3 mask = vec3(0.7);
	if (sub == 0) {
		mask.r = 1.0;
	} else if (sub == 1) {
		mask.g = 1.0;
	} else {
		mask.b = 1.0;
	}

	// Grid lines between pixels
	vec2 grid = abs(fract(pixel_pos) - 0.5) * 2.0;
	float grid_line = max(grid.x, grid.y);
	grid_line = smoothstep(0.8, 1.0, grid_line);
	float grid_factor = mix(1.0, 1.0 - grid_strength, grid_line);

	col.rgb *= mask * grid_factor * brightness_boost;

	COLOR = col;
}
