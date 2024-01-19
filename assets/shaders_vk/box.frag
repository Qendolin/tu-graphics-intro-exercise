#version 450

layout(location = 0) in vec3 in_direction;
layout(location = 0) out vec4 out_color;

layout (binding = 7) uniform samplerCube u_evironment_texture;

// AgX color transform from https://www.shadertoy.com/view/dlcfRX
vec3 AgX(vec3 color) {
	// Input transform
	color = mat3(.842, .0423, .0424, .0784, .878, .0784, .0792, .0792, .879) * color;
	// Log2 space encoding
	color = clamp((log2(color) + 12.47393) / 16.5, vec3(0), vec3(1));
	// Apply sigmoid function approximation
	color = .5 + .5 * sin(((-3.11 * color + 6.42) * color - .378) * color - 1.44);
	// AgX look (optional)
    // Punchy
  	color = mix(vec3(dot(color, vec3(.216,.7152,.0722))), pow(color,vec3(1.35)), 1.4);

	// Eotf
    color = mat3(1.2, -.053, -.053, -.1, 1.15, -.1, -.1, -.1, 1.15) * color;
	return color;
}

void main() {
	// normalizing the vertex normal is important and actually makes a difference
	vec3 direction = normalize(in_direction);
	out_color.rgb = texture(u_evironment_texture, direction).rgb;
	out_color.a = 1.0;
	out_color.rgb = AgX(pow(out_color.rgb, vec3(3.0)) + out_color.rgb);
}