#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_position;

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

#define ORTHO_LIGHT_COUNT 3
struct DirectionalLight {
	vec4 direction;
	vec4 color;
};
layout(set = 0, binding = 3) uniform DirectionalLights {
	DirectionalLight u_directional_lights[ORTHO_LIGHT_COUNT];
};

#define POINT_LIGHT_COUNT 4
struct PointLight {
	vec4 position;
	vec4 color;
	vec4 attenuation;
};
layout(set = 0, binding = 4) uniform PointLights {
	PointLight u_point_lights[POINT_LIGHT_COUNT];
};

#define SPOT_LIGHT_COUNT 2
struct SpotLight {
	vec4 position;
	vec4 direction;
	vec4 color;
	vec4 attenuation;
};
layout(set = 0, binding = 5) uniform SpotLights {
	SpotLight u_spot_lights[SPOT_LIGHT_COUNT];
};


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

float spot_falloff(vec3 N, vec3 L, float outer, float inner) {
	float theta = dot(-normalize(N), normalize(L));
	float gamma = cos(outer);
	float epsilon = cos(inner) - cos(outer);
	return smoothstep(0.0, 1.0, (theta - gamma) / epsilon);
}

float fresnel_schlick(vec3 N, vec3 V, float ior)
{
	float R0 = (1 - ior) / (1 + ior);
	R0 = R0 * R0;
	float cosTheta = dot(N, normalize(V));
	if(cosTheta < 0.0) return 0.0;
	return R0 + (1.0 - R0) * pow(max(0.0, 1.0 - cosTheta), 5.0);
}

void main() {
	gl_Position = u_view_projection_mat * u_model_mat * vec4(in_position, 1.0);
	vec3 color = in_color.rgb * mix(vec3(1.0), u_color.rgb, u_color.a);
	out_normal = normalize(mat3(transpose(inverse(u_model_mat))) * in_normal);

	vec3 P = (u_model_mat * vec4(in_position, 1.0)).xyz;
	vec3 V = u_camera_position.xyz - P;

	out_position = P;

	if(dot(V, out_normal) <= 0.0) {
		vec3 N = -out_normal;
		out_normal = N;

		vec3 ambient = vec3(1.0, 1.0, 1.0) * 0.01;
		vec3 diffuse = vec3(0.0);
		vec3 specular = vec3(0.0);

		float F = fresnel_schlick(N, V, 1.3);
		float kA = u_material_factors.x;
		float kD = u_material_factors.y;
		float kS = u_material_factors.z;
		float alpha = u_material_factors.w;

		for(int i = 0; i < ORTHO_LIGHT_COUNT; i++) {
			DirectionalLight light = u_directional_lights[i];
			vec3 L = -light.direction.xyz;
			diffuse += ortho_diffuse(N, L) * light.color.rgb * light.color.a;
			specular += ortho_specular(N, L, V, alpha) * light.color.rgb * light.color.a;
		}

		for(int i = 0; i < POINT_LIGHT_COUNT; i++) {
			PointLight light = u_point_lights[i];
			vec3 L = light.position.xyz - P;
			diffuse += point_diffuse(N, L, light.attenuation) * light.color.rgb * light.color.a;
			specular += point_specular(N, L, V, alpha) * light.color.rgb * light.color.a;
		}

		for(int i = 0; i < SPOT_LIGHT_COUNT; i++) {
			SpotLight light = u_spot_lights[i];
			vec3 L = light.position.xyz - P;
			float spot = spot_falloff(light.direction.xyz, L, light.direction.w, light.attenuation.w);
			diffuse += point_diffuse(N, L, light.attenuation) * light.color.rgb * light.color.a * spot;
			specular += point_specular(N, L, V, alpha) * light.color.rgb * light.color.a * spot;
		}
		
		specular += F * vec3(0.8, 1.0, 1.0);

		vec3 I = vec3(0.0);
		I += kA * ambient * in_color.rgb;
		I += kD * diffuse * in_color.rgb;
		I += kS * specular;
		out_color.rgb = I;
		out_color.rgb /= (out_color.rgb + 1.0);
	} else {
		out_color.rgb = color;
	}
	// reinhard tone mapping
	out_color.rgb /= (out_color.rgb + 1.0);
}