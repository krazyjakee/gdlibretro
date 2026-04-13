shader_type canvas_item;

uniform float scanline_strength : hint_range(0.0, 1.0) = 0.3;
uniform float curvature : hint_range(0.0, 0.1) = 0.03;
uniform float vignette_strength : hint_range(0.0, 1.0) = 0.4;
uniform float bloom_strength : hint_range(0.0, 1.0) = 0.15;

vec2 barrel_distort(vec2 uv) {
	vec2 centered = uv - 0.5;
	float r2 = dot(centered, centered);
	centered *= 1.0 + curvature * r2;
	return centered + 0.5;
}

void fragment() {
	vec2 uv = barrel_distort(UV);

	// Discard pixels outside the curved screen area
	if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
		COLOR = vec4(0.0, 0.0, 0.0, 1.0);
	} else {
		vec4 col = texture(TEXTURE, uv);

		// Bloom: sample neighbors and blend
		vec2 texel = 1.0 / vec2(textureSize(TEXTURE, 0));
		vec4 bloom = vec4(0.0);
		bloom += texture(TEXTURE, uv + vec2(-texel.x, 0.0));
		bloom += texture(TEXTURE, uv + vec2(texel.x, 0.0));
		bloom += texture(TEXTURE, uv + vec2(0.0, -texel.y));
		bloom += texture(TEXTURE, uv + vec2(0.0, texel.y));
		bloom *= 0.25;
		col.rgb = mix(col.rgb, max(col.rgb, bloom.rgb), bloom_strength);

		// Scanlines
		float scanline = sin(uv.y * float(textureSize(TEXTURE, 0).y) * 3.14159) * 0.5 + 0.5;
		col.rgb *= mix(1.0, scanline, scanline_strength);

		// Vignette
		vec2 vig = uv * (1.0 - uv);
		float vignette = vig.x * vig.y * 15.0;
		vignette = pow(vignette, vignette_strength * 0.5);
		col.rgb *= vignette;

		COLOR = col;
	}
}
