#version 450

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec3 in_normal;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 2) uniform Constants
{
	ivec4 u_user_input;
};


void main() {
	// normalizing the vertex normal is important and actually makes a difference
	vec3 normal = normalize(in_normal);
	if(u_user_input.x == 0) {
		out_color.rgb = in_color;
	} else if(u_user_input.x == 1) {
		out_color.rgb = normal;
	} else if(u_user_input.x == 2) {
		out_color.rgb = normal * 0.5 + 0.5;
	}
	out_color.a = 1.0;
}