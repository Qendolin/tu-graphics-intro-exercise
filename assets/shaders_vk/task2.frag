#version 450

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform UniformBlock
{
	vec4 u_color;
};

void main() {
	out_color = u_color;
}