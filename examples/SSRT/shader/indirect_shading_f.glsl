#version 430 core

layout(binding = 0,rgba32f) uniform image2D GBufferDiffuse;

layout(binding = 1,rgba32f) uniform image2D GBufferPos;

layout(binding = 2,rgba32f) uniform image2D GBufferNormal;

layout(binding = 3, r32f) uniform image2D GBufferDepth;

layout(binding = 4, r32f) uniform image2D GBufferShadow;

layout(binding = 5, rgba8) uniform image2D DirectColor;

layout(location = 0) out vec4 IndirectColor;

#define M_PI 3.1415926535897932384626433832795
#define TWO_PI 6.283185307
#define INV_PI 0.31830988618
#define INV_TWO_PI 0.15915494309

float Rand1(inout float p) {
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}
vec2 Rand2(inout float p) {
    return vec2(Rand1(p), Rand1(p));
}
vec3 SampleHemisphereUniform(inout float s, out float pdf) {
    vec2 uv = Rand2(s);
    float z = uv.x;
    float phi = uv.y * TWO_PI;
    float sinTheta = sqrt(1.0 - z*z);
    vec3 dir = vec3(sinTheta * cos(phi), sinTheta * sin(phi), z);
    pdf = INV_TWO_PI;
    return dir;
}


void LocalBasis(vec3 n, out vec3 b1, out vec3 b2) {
    float sign_ = sign(n.z);
    if (n.z == 0.0) {
        sign_ = 1.0;
    }
    float a = -1.0 / (sign_ + n.z);
    float b = n.x * n.y * a;
    b1 = vec3(1.0 + sign_ * n.x * n.x * a, sign_ * b, -sign_ * n.x);
    b2 = vec3(b, sign_ + n.y * n.y * a, -n.y);
}
float InitRand(vec2 uv) {
    vec3 p3  = fract(vec3(uv.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}
uniform mat4 CameraMVP;

uniform vec3 LightDir;
uniform vec3 CameraPos;
uniform vec3 LightRadiance;
uniform ivec2 window;
void main() {
    vec3 frag_diffuse = imageLoad(GBufferDiffuse,ivec2(gl_FragCoord)).rgb;
    vec3 direct_color = imageLoad(DirectColor,ivec2(gl_FragCoord)).rgb;
    vec3 frag_normal = normalize(imageLoad(GBufferNormal,ivec2(gl_FragCoord)).rgb);
    float frag_depth = imageLoad(GBufferDepth,ivec2(gl_FragCoord)).r;
    vec3 frag_pos = imageLoad(GBufferPos,ivec2(gl_FragCoord)).rgb;

    vec3 view_dir = normalize(CameraPos-frag_pos);
    float s = InitRand(gl_FragCoord.xy);
    int sample_count = 64;
    for(int i=0;i<sample_count;i++)
    {
        float pdf;
        vec3 ray_dir = SampleHemisphereUniform(s,pdf);
        vec3 b1,b2;
        LocalBasis(frag_normal,b1,b2);
        ray_dir = normalize(mat3(b1,b2,frag_normal)*ray_dir);


        vec3 end_pos = frag_pos;
        vec3 end_color = vec3(0.f);
        vec3 end_normal = vec3(0.f);
        float step = 0.5;
        for (int i=0;i<100;i++){
            end_pos = end_pos + step * ray_dir;
            vec4 proj_coord = CameraMVP * vec4(end_pos, 1.f);
            proj_coord.xyz = proj_coord.xyz / proj_coord.w;
            vec3 ndc = proj_coord.xyz * 0.5f + 0.5f;
            //        if(ndc.x<0.f || ndc.x>1.f || ndc.y<0.f || ndc.y>1.f || ndc.z<0.f || ndc.z>1.f) discard;
            int coord_x = int(ndc.x * window.x);
            int coord_y = int((ndc.y) * window.y);
            float test_depth = imageLoad(GBufferDepth, ivec2(coord_x, coord_y)).r;
            if (ndc.z > test_depth + 1e-5){
                end_normal = normalize(imageLoad(GBufferNormal, ivec2(coord_x, coord_y)).xyz);
                end_color = imageLoad(DirectColor, ivec2(coord_x, coord_y)).rgb;
                break;
            }
        }
        vec3 indirect_light_dir = normalize(end_pos-frag_pos);
        float diff = max(dot(end_normal, indirect_light_dir), 0.f) ;
        IndirectColor.rgb +=   end_color * frag_diffuse / pdf;
    }
    IndirectColor.rgb /= sample_count;
    IndirectColor.a = 1.f;
}
