#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec3 out_normal;

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
layout(set = 0, binding = 3) uniform DirectionalLight
{
	vec4 direction;
	vec4 color;
} u_directional_light;
layout(set = 0, binding = 4) uniform PointLight
{
	vec4 position;
	vec4 color;
	vec4 attenuation;
} u_point_light;

float point_diffuse(vec3 N, vec3 L, vec4 a) {
	float angle = max(0.0, dot(N, normalize(L)));
	float d = length(L);
	float i = a.x + a.y * d + a.z * d * d;
	return angle / max(i, 0.0001);
}

float point_specular(vec3 N, vec3 L, vec3 V, float alpha) {
	vec3 R = reflect(normalize(-L), N);
	float angle = max(0.0, dot(R, normalize(V)));
	return pow(angle, alpha);
}

float ortho_diffuse(vec3 N, vec3 L) {
	return max(0.0, dot(N, normalize(L)));
}

void main() {
	gl_Position = u_view_projection_mat * u_model_mat * vec4(in_position, 1.0);
	out_color.rgb = in_color.rgb * mix(vec3(1.0), u_color.rgb, u_color.a);
	out_normal = normalize(mat3(transpose(inverse(u_model_mat))) * in_normal);

	vec3 position = (u_model_mat * vec4(in_position, 1.0)).xyz;
	vec3 V = u_camera_position.xyz - position;

	vec3 ambient = vec3(1.0, 1.0, 1.0) * 0.01;
	vec3 diffuse = vec3(0.0);
	diffuse += ortho_diffuse(out_normal, -u_directional_light.direction.xyz) * u_directional_light.color.rgb * u_directional_light.color.a;
	diffuse += point_diffuse(out_normal, u_point_light.position.xyz - position, u_point_light.attenuation) * u_point_light.color.rgb * u_point_light.color.a;
	vec3 specular = vec3(0.0);
	specular += point_specular(out_normal, u_point_light.position.xyz - position, V, u_material_factors.w) * u_point_light.color.rgb * u_point_light.color.a;
	vec3 I = u_material_factors.x * ambient + u_material_factors.y * diffuse + u_material_factors.z * specular;
	out_color.rgb *= I;
}