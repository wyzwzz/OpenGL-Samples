#version 460 core
layout(location = 0) in vec3 inWorldPos;
layout(location = 0) out vec3 outFragColor;

uniform samplerCube environmentMap;
uniform float roughness;

const float PI = 3.14159265359;
const uint SampleCount = 1024u;

#define PrefilterMapSize 512.0

// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
// Van Der Corput 序列 把十进制数字的二进制表示镜像翻转到小数点右边而得
float RadicalInverse_Vdc(uint bits){
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
vec2 Hammersley(uint i, uint N){
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

float DistributionGGX(vec3 N, vec3 H, float roughness){

}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness){
    float a = roughness * roughness;

    float phi = 2.f * PI * Xi.x; // phi是xy平面内与x轴的夹角
    float cosTheta = sqrt((1.f - Xi.y) / (1.f + (a*a - 1.f) * Xi.y));
    float sinTheta = sqrt(1.f - cosTheta * cosTheta);

    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    //将H从局部坐标系转换到世界坐标系
    vec3 up = abs(N.z) < 0.999f ? vec3(0.f,0.f,1.f) : vec3(1.f,0.f,0.f);//防止与N平行 要cross运算
    vec3 tangent = normalize(cross(up,N));
    vec3 bitangent = cross(N,tangent);

    vec3 sample_vec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sample_vec);
}

void main(){
    //预滤波存储每个法向量对应的反射环境光 但是因为不知道观察方向 那么假设观察方向等于N
    //这个假设对于球的效果很好
    vec3 N = normalize(inWorldPos);

    //观察方向与物体表面法向量的夹角 会决定不一样的反射lobe形状 为了简单起见 假设观察方向等于出射方向等于法向量
    vec3 R = N;//出射方向
    vec3 V = R;//观察方向

    vec3 prefiltered_color = vec3(0.f);
    float total_weight = 0.f;

    for(uint i = 0u; i < SampleCount; i++){
        vec2 Xi = Hammersley(i,SampleCount);//生成随机数
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);//根据随机数和粗糙度生成 lobe内的半程向量
        vec3 L = normalize(2.f * dot(V,H) * H - V);//L是全球随机分布的 H是半球随机分布的

        float NdotL = max(dot(N,L), 0.f);
        if(NdotL > 0.f){
            //简单这样子计算 会造成明亮区域周围出现点状图案
            //prefiltered_color += texture(environmentMap,L).rgb * NdotL;
            //total_weight += NdotL;

            float D = DistributionGGX(N,H,roughness);
            float NdotH = max(dot(N, H), 0.f);
            float HdotV = max(dot(H, V), 0.f);
            float pdf = D * NdotH / (4.f * HdotV) + 0.0001f;

            float resolution = PrefilterMapSize;
            float sa_texel = 4.f * PI / (6.f * resolution *resolution);
            float sa_sample = 1.f / (float(SampleCount) * pdf + 0.0001f);

            float mip_level = roughness == 0.f ? 0.f : 0.5f * log2(sa_sample / sa_texel);

            prefiltered_color += textureLoad(environmentMap, L, mip_level).rgb * NdotL;
            total_weight += NdotL;
        }
    }

    prefiltered_color /= total_weight;
    
    outFragColor = vec4(prefiltered_color,1.f);
}