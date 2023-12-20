#version 450 core

layout(location = 0) in vec3 in_albedo;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_position;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform CameraUniforms
{
	mat4 u_view_projection_mat;
	vec4 u_camera_position;
};

#define ORTHO_LIGHT_COUNT 2
struct DirectionalLight {
	vec4 direction;
	vec4 color;
};
layout(set = 0, binding = 3) uniform DirectionalLights {
	DirectionalLight u_directional_lights[ORTHO_LIGHT_COUNT];
};

#define POINT_LIGHT_COUNT 2
struct PointLight {
	vec4 position;
	vec4 color;
	vec4 attenuation;
};
layout(set = 0, binding = 4) uniform PointLights {
	PointLight u_point_lights[POINT_LIGHT_COUNT];
};


layout(set = 0, binding = 1) uniform ModelUniforms
{
	vec4 u_color;
	mat4 u_model_mat;
	vec4 u_material_factors;
};

const float PI = 3.14159265359;

float distribution_ggx(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    // when roughness is zero and N = H denom would be 0
    denom = PI * denom * denom + 5e-6;

    return nom / denom;
}

float geometry_schlick_ggx(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness)
{
    // + 5e-6 to prevent artifacts, value is from https://google.github.io/filament/Filament.html#materialsystem/specularbrdf:~:text=float%20NoV%20%3D%20abs(dot(n%2C%20v))%20%2B%201e%2D5%3B
    float NdotV = max(dot(N, V), 0.0) + 5e-6;
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometry_schlick_ggx(NdotV, roughness);
    float ggx1 = geometry_schlick_ggx(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnel_schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    vec3 albedo     = in_albedo;
    float metallic  = u_material_factors.z;
    float roughness = u_material_factors.y;

    vec3 N = normalize(in_normal);
    vec3 P = in_position;
    vec3 V = normalize(u_camera_position.xyz - P);
    vec3 R = reflect(-V, N);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < ORTHO_LIGHT_COUNT + POINT_LIGHT_COUNT; ++i)
    {
        vec3 L, radiance;
        if(i < ORTHO_LIGHT_COUNT) {
            DirectionalLight light = u_directional_lights[i];
            L = normalize(-light.direction.xyz);
            radiance = light.color.rgb * light.color.a;
        } else {
            PointLight light = u_point_lights[i-ORTHO_LIGHT_COUNT];
            L = normalize(light.position.xyz - P);
            float d = length(light.position.xyz - P) + 1e-5;
            float attenuation = light.attenuation.x + light.attenuation.y * d + light.attenuation.z * d * d;
            radiance = (light.color.rgb * light.color.a) / attenuation;
        }
        vec3 H = normalize(V + L);

        // Cook-Torrance BRDF
        float NDF = distribution_ggx(N, H, roughness);
        float G   = geometry_smith(N, V, L, roughness);
        vec3 F    = fresnel_schlick(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 1e-5; // + 1e-5 to prevent divide by zero
        vec3 specular = numerator / denominator;

        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    // ambient lighting
    vec3 kS = fresnel_schlick(max(dot(N, V), 0.0), F0);
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	  
    vec3 irradiance = vec3(0.8, 1.0, 1.0) * 0.03;
    vec3 diffuse = irradiance * albedo;
    vec3 ambient = (kD * diffuse);

    vec3 color = ambient + Lo;

    out_color = vec4(color, 1.0);
    // reinhard tone mapping
	out_color.rgb /= (out_color.rgb + 1.0);
}