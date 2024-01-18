#version 450

layout(location = 0) in vec3 in_direction;
layout(location = 0) out vec4 out_color;

layout (binding = 7) uniform samplerCube u_evironment_texture;

void main() {
	// normalizing the vertex normal is important and actually makes a difference
	vec3 direction = normalize(in_direction);
	out_color.rgb = texture(u_evironment_texture, direction).rgb;
	out_color.a = 1.0;
}