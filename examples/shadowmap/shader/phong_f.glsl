#version 430 core
in vec3 position;
in vec3 normal;
in vec2 tex_coord;
in vec4 pos_from_light;
out vec4 frag_color;

uniform sampler2D ShadowMap;
uniform vec3 light_dir;
uniform vec3 camera_pos;

float unpack(vec4 rgbaDepth) {
    const vec4 bitShift = vec4(1.0, 1.0/256.0, 1.0/(256.0*256.0), 1.0/(256.0*256.0*256.0));
    return dot(rgbaDepth, bitShift);
}
float Bias(){
    //解决shadow bias 因为shadow map的精度有限，当要渲染的fragment在light space中距Light很远的时候，
//就会有多个附近的fragement会samper shadow map中同一个texel,
//但是即使这些fragment在camera view space中的深度值z随xy变化是值变化是很大的，
    //但他们在light space 中的z值(shadow map中的值)却没变或变化很小，这是因为shadow map分辨率低，采样率低导致精度低，不能准确的记录这些细微的变化

    // calculate bias (based on depth map resolution and slope)  vec3 lightDir = normalize(uLightPos);
    vec3 lightDir = -normalize(light_dir);
    vec3 n = normalize(normal);
    float bias = max(0.005 * (1.0 - dot(n, lightDir)), 0.0001);
    return  bias;
}

float useShadowMap(sampler2D shadow_map,vec4 shadow_coord){
    float  bias = Bias();
    vec4 depth_pack = texture(shadow_map,shadow_coord.xy);
//    float depth_unpack = unpack(depth_pack);
    float depth_unpack = depth_pack.x;
    if(depth_unpack > shadow_coord.z -bias){
        return 1.f;
    }
    return 0.f;
}
vec3 blinnPhong(){
    vec3 color = vec3(0.8f,0.9f,1.f);

    vec3 ambient = 0.15*color;

    vec3 n = normalize(normal);
    float diff = max(dot(-light_dir,n),0.f);

    vec3 diffuse = diff * color;

    vec3 view_dir = normalize(camera_pos- position);
    vec3 half_dir = normalize(view_dir + -light_dir);
    float spec = pow(max(dot(half_dir,n),0.f),32.f);
    vec3 specular =  spec * vec3(1.f,1.f,1.f);

    vec3 radiance = ambient + diffuse + specular;
    vec3 phong_color = pow(radiance,vec3(1.f/2.2f));
    return phong_color;

}
void main() {

    float visibility;
    vec3 proj_coord = pos_from_light.xyz / pos_from_light.w;
    vec3 shadowCoord = proj_coord*0.5f +0.5f;
//    frag_color = vec4(shadowCoord,1.f);return;
    visibility = useShadowMap(ShadowMap,vec4(shadowCoord,1.f));

    vec3 phongColor = blinnPhong();

    frag_color = vec4(phongColor*visibility,1.f);
}
