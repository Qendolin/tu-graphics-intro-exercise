#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 1) uniform ModelUniforms
{
	vec4 u_color;
	mat4 u_model_mat;
};
layout(set = 0, binding = 0) uniform CameraUniforms
{
	mat4 u_view_projection_mat;
};
layout(set = 0, binding = 2) uniform Constants
{
	ivec4 u_user_input;
};

void main() {
	gl_Position = u_view_projection_mat * u_model_mat * vec4(in_position, 1.0);
	if(u_user_input.x == 0) {
		out_color.rgb = in_color.rgb * mix(vec3(1.0), u_color.rgb, u_color.a);
	} else if(u_user_input.x == 1) {
		out_color.rgb = in_normal;
	} else if(u_user_input.x == 2) {
		out_color.rgb = in_normal * 0.5 + 0.5;
	}
	out_color.a = 1.0;
}