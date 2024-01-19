#version 450

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_position;
layout(location = 3) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 2) uniform Constants
{
	ivec4 u_user_input;
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
}
u_directional_light;
layout(set = 0, binding = 4) uniform PointLight
{
	vec4 position;
	vec4 color;
	vec4 attenuation;
}
u_point_light;
layout(set = 0, binding = 1) uniform ModelUniforms
{
	vec4 u_color;
	mat4 u_model_mat;
	vec4 u_material_factors;
};

layout (binding = 5) uniform sampler2D diffuse_texture;
layout (binding = 6) uniform sampler2D specular_texture;
layout (binding = 7) uniform samplerCube evironment_texture;

float point_diffuse(vec3 N, vec3 L, vec4 a)
{
	float angle = max(0.0, dot(N, normalize(L)));
	float d = length(L);
	float i = a.x + a.y * d + a.z * d * d;
	return angle / max(i, 0.0001);
}

float point_specular(vec3 N, vec3 L, vec3 V, float alpha)
{
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

float ortho_diffuse(vec3 N, vec3 L)
{
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

// AgX color transform from https://www.shadertoy.com/view/dlcfRX
vec3 AgX(vec3 color) {
	// Input transform
	color = mat3(.842, .0423, .0424, .0784, .878, .0784, .0792, .0792, .879) * color;
	// Log2 space encoding
	color = clamp((log2(color) + 12.47393) / 16.5, vec3(0), vec3(1));
	// Apply sigmoid function approximation
	color = .5 + .5 * sin(((-3.11 * color + 6.42) * color - .378) * color - 1.44);
	// AgX look (optional)
    // Punchy
  	color = mix(vec3(dot(color, vec3(.216,.7152,.0722))), pow(color,vec3(1.35)), 1.4);

	// Eotf
    color = mat3(1.2, -.053, -.053, -.1, 1.15, -.1, -.1, -.1, 1.15) * color;
	return color;
}

void main()
{
	// normalizing the vertex normal is important and actually makes a difference
	vec3 N = normalize(in_normal);
	out_color.a = 1.0;
	if (u_user_input[0] == 1)
	{
		out_color.rgb = N;
		return;
	}
	else if (u_user_input[0] == 2)
	{
		out_color.rgb = N * 0.5 + 0.5;
		return;
	}
	if(u_user_input[1] == 1) {
		out_color.rgb = vec3(in_uv, 0.0);
		return;
	}

	vec3 P = in_position;
	vec3 V = u_camera_position.xyz - P;

	vec3 ambient = vec3(1.0, 1.0, 1.0) * 0.1;
	vec3 diffuse = vec3(0.0);
	vec3 specular = vec3(0.0);

	vec3 L = -u_directional_light.direction.xyz;
	diffuse += ortho_diffuse(N, L) * u_directional_light.color.rgb * u_directional_light.color.a;
	specular += ortho_specular(N, L, V, u_material_factors.w) * u_directional_light.color.rgb * u_directional_light.color.a;
	
	L = u_point_light.position.xyz - P;
	diffuse += point_diffuse(N, L, u_point_light.attenuation) * u_point_light.color.rgb * u_point_light.color.a;
	specular += point_specular(N, L, V, u_material_factors.w) * u_point_light.color.rgb * u_point_light.color.a;
	
	vec3 diffuse_color = texture(diffuse_texture, in_uv).rgb * in_color.rgb;
	vec3 specular_color = texture(specular_texture, in_uv).rgb;
	vec3 environment_color = texture(evironment_texture, normalize(reflect(-V, N))).rgb;
	environment_color = pow(environment_color.rgb, vec3(3.0)) + environment_color.rgb;

	float kS = u_material_factors.z * 2.0;
	float kD = u_material_factors.y;

	vec3 I = vec3(0.0);
	I += u_material_factors.x * ambient * diffuse_color;
	I += kD * diffuse * diffuse_color;
	I += kS * specular * specular_color;
	I += kS * environment_color * specular_color;
	out_color.rgb = I;

	out_color.rgb = AgX(out_color.rgb);
}