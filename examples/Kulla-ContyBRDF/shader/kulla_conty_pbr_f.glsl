#version 460 core
layout(location = 0) in vec3 inFragPos;
layout(location = 1) in vec3 inFragNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 outFragColor;

uniform vec3 uAlbedo; //all the same for model
uniform float uMetallic;
uniform float uRoughness;
uniform sampler2D uBRDFLut;
uniform sampler1D uEavgLut;

uniform vec3 uLightPos;
uniform vec3 uLightRadiance;

uniform vec3 uCameraPos;

uniform bool uKC;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness){
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N,H),0.f);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.f) + 1.f);
    denom = PI * denom * denom;

    return nom / denom;
}
float GeometrySchlickGGX(float NdotV, float roughness){
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness){
    float NdotV = max(dot(N,V),0.f);
    float NdotH = max(dot(N,L),0.f);
    float ggx2 = GeometrySchlickGGX(NdotV,roughness);
    float ggx1 = GeometrySchlickGGX(NdotH,roughness);

    return ggx1 * ggx2;
}
vec3 fresnelSchlick(float cosTheta, vec3 F0){
    return F0 + (1.f - F0) * pow(max(1.f - cosTheta, 0.f), 5.f);
}

//https://blog.selfshadow.com/publications/s2017-shading-course/imageworks/s2017_pbs_imageworks_slides_v2.pdf
vec3 AverageFresnel(vec3 r, vec3 g)
{
    return vec3(0.087237) + 0.0230685*g - 0.0864902*g*g + 0.0774594*g*g*g
           + 0.782654*r - 0.136432*r*r + 0.278708*r*r*r
           + 0.19744*g*r + 0.0360605*g*g*r - 0.2586*g*r*r;
}

vec3 MultiScatterBRDF(float NdotL, float NdotV)
{
    vec3 albedo = pow(uAlbedo,vec3(2.2f));

    vec3 E_o = texture(uBRDFLut,vec2(NdotL, uRoughness)).xxx;
    vec3 E_i = texture(uBRDFLut,vec2(NdotV, uRoughness)).xxx;

    vec3 E_avg = texture(uEavgLut,uRoughness).xxx;

    vec3 edge_tint = vec3(0.827f,0.792f,0.678f);
    vec3 F_avg = AverageFresnel(albedo, edge_tint);

    vec3 fms = (1.f - E_o) * (1.f - E_i) / (PI * (1.0 - E_avg));
    vec3 fadd = F_avg * E_avg / (1.0 - F_avg * (1.0 - E_avg));

    return fms * fadd;
}

void main(){
    vec3 albedo = pow(uAlbedo,vec3(2.2f));
    vec3 N = normalize(inFragNormal);
    vec3 V = normalize(uCameraPos - inFragPos);
    float NdotV = max(dot(N,V),0.f);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0,albedo,uMetallic);

    vec3 Lo = vec3(0.f);

    vec3 L = normalize(uLightPos);
    vec3 H = normalize(V + L);
    // float distance = length(uLightPos - inFragPos);
    // float attenuation = 1.f / (distance * distance);
    vec3 radiance = uLightRadiance ;//* attenuation;

    float NDF = DistributionGGX(N,H,uRoughness);
    float G = GeometrySmith(N,V,L,uRoughness);
    vec3 F = fresnelSchlick(max(dot(H,V),0.f),F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.f * max(dot(N,V),0.f) * max(dot(N,L),0.f);
    vec3 Fmicro = numerator / max(denominator,0.001f);

    float NdotL = max(dot(N,L),0.f);

    vec3 Fms = vec3(0.f);
    
    if(uKC)
        Fms = MultiScatterBRDF(NdotL,NdotV);
    
    vec3 BRDF = Fmicro + Fms;
    
    Lo += BRDF * radiance * NdotL;;
    vec3 color = Lo;

    color = color / (color + vec3(1.f));
    color = pow(color, vec3(1.f/2.2f));
    
    outFragColor = vec4(color,1.f);
}