#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec2 in_uv;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_position;
layout(location = 3) out vec2 out_uv;

layout(set = 0, binding = 1) uniform ModelUniforms
{
	vec4 u_color;
	mat4 u_model_mat;
	vec4 u_material_factors;
};
layout(set = 0, binding = 0) uniform CameraUniforms
{
	mat4 u_view_projection_mat;
	vec4 u_camera_position;
};

void main() {
	gl_Position = u_view_projection_mat * u_model_mat * vec4(in_position, 1.0);
	out_color.rgb = in_color.rgb * mix(vec3(1.0), u_color.rgb, u_color.a);
	out_normal = normalize(mat3(transpose(inverse(u_model_mat))) * in_normal);
	out_position = (u_model_mat * vec4(in_position, 1.0)).xyz;
	out_uv = in_uv;
}