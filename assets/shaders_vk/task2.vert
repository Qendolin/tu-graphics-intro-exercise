#version 450

layout(location = 0) in vec3 in_position;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform ModelUniforms
{
	vec4 u_color;
	mat4 u_model_mat;
};
layout(binding = 1) uniform CameraUniforms
{
	mat4 u_view_projection_mat;
};

void main() {
	gl_Position = u_view_projection_mat * u_model_mat * vec4(in_position, 1.0);
	out_color = u_color;
}