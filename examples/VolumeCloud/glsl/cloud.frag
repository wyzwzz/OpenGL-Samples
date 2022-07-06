#version 460 core
layout(location = 0) in vec2 iUV;
layout(location = 0) out vec4 oFragColor;

uniform sampler2D GDepth;
uniform sampler2D GColor;

uniform sampler2D WeatherMap;

uniform sampler3D ShapeNoise;
uniform sampler3D DetailNoise;

uniform sampler2D BlueNoise;

uniform mat4 InvProj;
uniform mat4 InvView;
uniform vec3 CameraPos;//4.087f,3.7f,3.957f
uniform vec3 LightDir;
uniform vec3 LightRadiance;

vec4 getWorldPos(vec3 xyz){
    vec4 view_p = InvProj * vec4(xyz * 2.0 - 1.0,1.0);
    view_p /= view_p.w;
    return InvView * view_p;
}

bool insideBox(vec3 pos,vec3 box_min,vec3 box_max){
    return pos.x >= box_min.x && pos.x <= box_max.x
        && pos.y >= box_min.y && pos.y <= box_max.y
        && pos.z >= box_min.z && pos.z <= box_max.z;
}

vec2 rayIntersectBox(vec3 box_min,vec3 box_max,vec3 ray_origin,vec3 inv_ray_dir){
    vec3 t0 = (box_min - ray_origin) * inv_ray_dir;
    vec3 t1 = (box_max - ray_origin) * inv_ray_dir;
    vec3 t_min = min(t0,t1);
    vec3 t_max = max(t0,t1);
    float enter_t = max(max(t_min.x,t_min.y),t_min.z);
    float exit_t  = min(min(t_max.x,t_max.y),t_max.z);
    return vec2(enter_t,exit_t);//exit_t > enter_t && exit_t > 0
}

//const vec3 box_min = vec3(-2,4,-2);
//const vec3 box_max = vec3(2,6,2);

const vec3 box_min = vec3(-18,16,-18);
const vec3 box_max = vec3(18,24,18);

float remap(float original_value, float original_min, float original_max, float new_min, float new_max)
{
    return new_min + (((original_value - original_min) / (original_max - original_min)) * (new_max - new_min));
}

float saturate(float x){
    return clamp(x,0.0,1.0);
}

vec2 saturate(vec2 x){
    return clamp(x,vec2(0.0),vec2(1.0));
}

vec3 saturate(vec3 x){
    return clamp(x,vec3(0.0),vec3(1.0));
}
#define sampleDensity sampleDensity1


const float shape_tiling = 1;

float sampleDensity0(vec3 pos){
    vec3 box_size = box_max - box_min;
    vec3 box_center = 0.5 * (box_min + box_max);
    vec2 uv = saturate((pos.xz - box_min.xz) / box_size.xz);
    float weather = texture(WeatherMap,uv).r;
    float height_percent = saturate((pos.y - box_min.y) / box_size.y);
    float g_min = remap((1 - weather) * weather, 0, 1, 0.1, 0.6);
    float g_max = remap(weather, 0, 1, g_min, 0.9);
    float height_gradient1 = saturate(remap(height_percent,0.0,g_min,0,1)) * saturate(remap(height_percent,1,g_max,0,1));
    float height_gradient2 = min(weather,saturate(remap(height_percent,0.0,weather,1.0,0.0)))
    * saturate(remap(height_percent,g_min,1,0,1));
    float height_gradient = saturate(mix(height_gradient1,height_gradient2,0.5));

    const float containerEdgeFadeDst = 4;
    float dstFromEdgeX = min(containerEdgeFadeDst, min(pos.x - box_min.x, box_max.x - pos.x));
    float dstFromEdgeZ = min(containerEdgeFadeDst, min(pos.z - box_min.z, box_max.z - pos.z));
    float edgeWeight = min(dstFromEdgeZ, dstFromEdgeX) / containerEdgeFadeDst;
//
    height_gradient2 *= edgeWeight;

//    vec3 uvw = pos * shape_tiling;
    float base_desnity = texture(ShapeNoise,pos * 0.52).r;

    return   height_gradient * texture(ShapeNoise,pos * 0.52).r ;//* 0.25;//height_gradient;// * texture(ShapeNoise,uvw).r;
}




const float g_c = 0.8;
const float g_d = 1;

float sampleDensity1(vec3 pos){
    vec3 box_size = box_max - box_min;
    vec3 box_center = 0.5 * (box_min + box_max);
    vec2 uv = saturate((pos.xz - box_min.xz) / box_size.xz);

    const float containerEdgeFadeDst = 3;
    float dstFromEdgeX = min(containerEdgeFadeDst, min(pos.x - box_min.x, box_max.x - pos.x));
    float dstFromEdgeZ = min(containerEdgeFadeDst, min(pos.z - box_min.z, box_max.z - pos.z));
    float edgeWeight = min(dstFromEdgeZ, dstFromEdgeX) / containerEdgeFadeDst;

    float wd = texture(WeatherMap,uv).r;

    const float wc0 = 0.8;
    const float wc1 = 0.9;
    const float WMc = max(wc0,saturate(g_c - 0.5) * wc1 * 2);
    float ph = saturate((pos.y - box_min.y) / box_size.y);
    const float wh = 0.8;

    float SRb = saturate(remap(ph,0,0.07,0,1));
    float SRt = saturate(remap(ph,wh*0.2,wh,1,0));
    float SA = SRb * SRt;

    float DRb = ph * saturate(remap(ph,0,0.15,0,1));
    float DRt = saturate(remap(ph,0.9,1.0,1,0));
    float DA = g_d * DRb * DRt * wd * 2;

    float SN = texture(ShapeNoise,pos * 0.1).r;

    SN = remap(SN * SA,1 - g_c * WMc,1, 0, 1) * DA * edgeWeight;

    float DN_fbm = texture(DetailNoise,pos * 0.22).r;
    float DN_mod = 0.35 * exp(-g_c * 0.75) * mix(DN_fbm,1 - DN_fbm,saturate(ph * 5));

    float d = saturate(remap(SN,DN_mod,1,0,1));
    return d;
}

const float density_to_sigma_t = 1;//0.15;

vec3 singleScattering(vec3 pos){
    vec3 ray_dir = -LightDir;
    vec2 intersect_t = rayIntersectBox(box_min,box_max,pos,1.0 / (ray_dir + vec3(0.001)));
    float ray_march_dist = max(0,intersect_t.y);
    int ray_march_step = 8;
    float dt = ray_march_dist / ray_march_step;
    float sum_sigma_t = 0;
    for(int i = 0; i < ray_march_step; ++i){
        vec3 ith_pos = pos + (i + 0.5) * ray_dir * dt;
        sum_sigma_t += sampleDensity(ith_pos) * density_to_sigma_t * dt;
    }
    return LightRadiance * exp(-sum_sigma_t);
}

#define PI 3.14159265
vec3 evalPhaseFunction(float u){

    float g = -0.8;
    float g2 = g * g;
    float u2 = u * u;
    float m = 1 + g2 - 2 * g * u;
    float pMie = 3 / (8 * PI) * (1 - g2) * (1 + u2) / ((2 + g2) * m * sqrt(m));

    vec3 pRayleigh = vec3(abs(3 / (16 * PI) * (1 + u2)));

    return (pMie * 0.5 + pRayleigh * 0.5) * 0.8 + 0.2;
}

vec3 densityToSigmaS(float density){
    return vec3(1) * density;
}
const vec2 ScreenUV = vec2(1920.0/512,1080/512);
vec3 cloudRayMarching(vec3 start_pos,vec3 ray_dir,float max_ray_advance_dist,vec3 in_radiance){

    if(max_ray_advance_dist < 0.001) return in_radiance;

    int ray_marching_step_count = 64;
    float sum_sigma_t = 0;
    float dt = 18.0 / 64;//max_ray_advance_dist / ray_marching_step_count;
    vec3 sum_radiance = vec3(0);

    float blue_noise = texture(BlueNoise,iUV).r;
    float travel_dist = dt * blue_noise ;
    for(int i = 0; i < ray_marching_step_count; ++i){
        if(travel_dist > max_ray_advance_dist) break;
        vec3 ith_pos = start_pos + travel_dist * ray_dir;
        //get radiance not transmittance
        vec3 ith_single_scattering = singleScattering(ith_pos);
        float density = sampleDensity(ith_pos);
        sum_sigma_t +=  density * density_to_sigma_t * dt;
        vec3 sigma_s = densityToSigmaS(density);
        vec3 rho = evalPhaseFunction(dot(LightDir,-ray_dir));
        vec3 ith_transmittance = vec3(exp(-sum_sigma_t));
        sum_radiance += ith_single_scattering * sigma_s * rho *  ith_transmittance * dt;
        if( all( lessThan( ith_transmittance,vec3(0.01) ) ) )
            break;
        travel_dist += dt;
    }
    sum_radiance += in_radiance * exp(-sum_sigma_t);
//    sum_radiance +=  1 - exp(-sum_sigma_t);
    return sum_radiance;
}
vec4 cloudRawCast(vec3 start_pos,vec3 ray_dir,float max_ray_advance_dist){
    int ray_cast_step_count = 512;
    float step = 18.0 / 500;
    float advanced_dist = texture(BlueNoise,iUV).r * step;
    vec4 color = vec4(0.0,0.0,0.0,0.0);

    for(int i = 0; i < ray_cast_step_count; ++i){

        vec3 ith_pos = start_pos + (advanced_dist + 0.5 * step) * ray_dir;

        float density = sampleDensity(ith_pos);

        vec4 denisty_color = vec4(density);

        color = color + denisty_color * vec4(density,density,density,1.0) * (1.0 - color.a);
        if(color.w > 0.99)
            break;
        advanced_dist += step;
        if(advanced_dist >= max_ray_advance_dist) break;
    }
    return color;
}

void main() {
    float depth = texture(GDepth,iUV).r;
    vec3 color = texture(GColor,iUV).rgb;
    vec3 world_pos = getWorldPos(vec3(iUV,depth)).xyz;
    vec3 camera_pos = CameraPos;

    vec3 cam_to_pos = world_pos - camera_pos;
    float cam_to_pos_dist = length(cam_to_pos);
    vec3 view_dir = normalize(cam_to_pos);
    vec2 intersect_t = rayIntersectBox(box_min,box_max,camera_pos,1.0 / view_dir);
    vec3 cloud = vec3(0);
    //intersect with box
    if(intersect_t.y > 0 && intersect_t.y > intersect_t.x){
        float cam_to_box_dist = max(0,intersect_t.x);
        float ray_box_dist = intersect_t.y - max(0,intersect_t.x);
        float ray_advance_dist = min(ray_box_dist,max(0,cam_to_pos_dist - cam_to_box_dist));
        vec3 radiance = cloudRayMarching(camera_pos + cam_to_box_dist * view_dir,view_dir,ray_advance_dist,color);
//        vec3 radiance = vec3( (camera_pos + cam_to_box_dist * view_dir)/ 20);
//        vec4 radiance = cloudRawCast(camera_pos + cam_to_box_dist * view_dir, view_dir,ray_advance_dist);
        cloud = mix(color,radiance.rgb,1);
    }
    else{
        cloud = color;
    }
    oFragColor = vec4(cloud,1.0);
//    oFragColor = vec4( mix(color,vec3(cloud),cloud),1.0);
//    oFragColor = vec4(world_pos/10,1.0);
}
