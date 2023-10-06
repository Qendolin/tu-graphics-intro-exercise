#version 450

layout(location = 0) in vec3 in_position;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform UniformBlock
{
	vec4 u_color;
	mat4 u_view_projection;
};

void main() {
	gl_Position = u_view_projection * vec4(in_position, 1.0);
	out_color = u_color;
}