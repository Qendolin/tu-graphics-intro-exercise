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

float ortho_specular(vec3 N, vec3 L, vec3 V, float alpha)
{
	vec3 R = reflect(normalize(-L), N);
	float angle = max(0.0, dot(R, normalize(V)));
	return pow(angle, alpha);
}

float ortho_diffuse(vec3 N, vec3 L) {
	return max(0.0, dot(N, normalize(L)));
}

float fresnel_schlick(vec3 N, vec3 V, float ior)
{
	float R0 = (1 - ior) / (1 + ior);
	R0 = R0 * R0;
	float cosTheta = dot(N, normalize(V));
	if(cosTheta < 0.0) return 0.0;
	return R0 + (1.0 - R0) * pow(max(0.0, 1.0 - cosTheta), 5.0);
}

vec3 getCornellBoxReflectionColor(vec3 positionWS, vec3 directionWS)
{
	vec3 P0 = positionWS;
	vec3 V = normalize(directionWS);
	const float boxSize = 1.5;
	vec4[5] planes = {
		vec4(-1.0, 0.0, 0.0, -boxSize), // left
		vec4(1.0, 0.0, 0.0, -boxSize),	// right
		vec4(0.0, 1.0, 0.0, -boxSize),	// top
		vec4(0.0, -1.0, 0.0, -boxSize), // bottom
		vec4(0.0, 0.0, -1.0, -boxSize)	// back
	};
	vec3[5] colors = {
		vec3(1.0, 0.0, 0.0),	// left
		vec3(0.0, 1.0, 0.0),	// right
		vec3(0.96, 0.93, 0.85), // top
		vec3(0.64, 0.64, 0.64), // bottom
		vec3(0.76, 0.74, 0.68)	// back
	};
	for (int i = 0; i < 5; ++i)
	{
		vec3 N = planes[i].xyz;
		float d = planes[i].w;
		float denom = dot(V, N);
		if (denom <= 0)
			continue;
		float t = -(dot(P0, N) + d) / denom;
		vec3 P = P0 + t * V;
		float q = boxSize + 0.01;
		if (P.x > -q && P.x < q && P.y > -q && P.y < q && P.z > -q && P.z < q)
		{
			return colors[i];
		}
	}
	return vec3(0.0, 0.0, 0.0);
}

vec3 clampedReflect(vec3 I, vec3 N) {
	return I - 2.0 * min(dot(N, I), 0.0) * N;
}

void main() {
	gl_Position = u_view_projection_mat * u_model_mat * vec4(in_position, 1.0);
	vec3 color = in_color.rgb * mix(vec3(1.0), u_color.rgb, u_color.a);
	out_normal = normalize(mat3(transpose(inverse(u_model_mat))) * in_normal);

	vec3 P = (u_model_mat * vec4(in_position, 1.0)).xyz;
	vec3 V = u_camera_position.xyz - P;
	vec3 N = out_normal;

	vec3 ambient = vec3(1.0, 1.0, 1.0);
	vec3 diffuse = vec3(0.0);
	vec3 specular = vec3(0.0);

	vec3 L = -u_directional_light.direction.xyz;
	diffuse += ortho_diffuse(N, L) * u_directional_light.color.rgb * u_directional_light.color.a;
	specular += ortho_specular(N, L, V, u_material_factors.w) * u_directional_light.color.rgb * u_directional_light.color.a;
	
	L = u_point_light.position.xyz - P;
	diffuse += point_diffuse(N, L, u_point_light.attenuation) * u_point_light.color.rgb * u_point_light.color.a;
	specular += point_specular(N, L, V, u_material_factors.w) * u_point_light.color.rgb * u_point_light.color.a;

	vec3 I = vec3(0.0);
	I += u_material_factors.x * ambient * color;
	I += u_material_factors.y * diffuse * color;
	I += u_material_factors.z * specular;
	I = mix(I, getCornellBoxReflectionColor(P, clampedReflect(normalize(-V), N)), fresnel_schlick(N, V, 1.925));
	out_color.rgb = I;
}