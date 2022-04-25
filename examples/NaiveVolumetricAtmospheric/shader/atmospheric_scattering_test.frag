#version 460 core
out vec4 oFragColor;
uniform vec2 WindowSize;//w h
uniform vec3 CameraPos;
uniform vec3 CameraRight;
uniform vec3 CameraUp;
uniform float CameraFov;//radians
uniform vec3 ViewDirection;
uniform vec3 LightDirection;//toward light
uniform vec3 LightRadiance;
const float PlanetRadius = 6.3710;
const float PlanetAtmosphereR = 6.4710;
const vec3 PlanetCenter = vec3(0.0,0.0,0.0);
const float ScaleHeight = 0.0085;
const float BetaR = 5.19673;
const float BetaG = 12.1427;
const float BetaB = 29.6453;
const float PhaseCoef = 3.0 / (16.0 * 3.14159265359);

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
const mat3 ACESInputMat =
{
{0.59719, 0.35458, 0.04823},
{0.07600, 0.90834, 0.01566},
{0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 ACESOutputMat =
{
{ 1.60475, -0.53108, -0.07367},
{-0.10208,  1.10813, -0.00605},
{-0.00327, -0.07276,  1.07602}
};
vec3 RRTAndODTFit(in vec3 v)
{
    vec3 a = v * (v + 0.0245786f) - 0.000090537f;
    vec3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}
//compute entry and exit pos with earth
bool ComputeRayIntersecT(in vec3 pos,in vec3 dir,out float start,out float end,in vec3 C,in float R){
    vec3 L = C - pos;
    float tca = dot(L,dir);
    float d2 = dot(L,L) - tca *tca;
    float R2 = R * R;
    if(d2 > R2) return false;
    float thc = sqrt(R2 - d2);
    start = tca - thc;
    end = tca + thc;
    return true;
}

bool LightSampling(in vec3 P,in vec3 S,out float opticalDepthCA){

    float time = 0.0;
    //compute P to atmosphere boundary
    float t_start,t_end;
    bool intersect = ComputeRayIntersecT(P,S,t_start,t_end,PlanetCenter,PlanetRadius);
    //if P is unable to see sun then return false
    if(intersect && t_end > 0.0) return false;
    bool ok = ComputeRayIntersecT(P,S,t_start,t_end,PlanetCenter,PlanetAtmosphereR);
    if(!ok) discard;
    float dist = distance(P,P+S*t_end);
    float light_sampling_step = 0.001;
//    float ds = dist / float(light_sampling_steps);
    int light_sampling_steps = int(dist / light_sampling_step);
    float ds = light_sampling_step;
    for(int i = 0; i < light_sampling_steps; i++){
        vec3 Q = P + S * (time + ds * 0.5);
        float height = distance(PlanetCenter,Q) - PlanetRadius;
        //this is redundant if test intersect
//        if( height < 0.0){
//            return false;
//        }
        //optical depth for the light ray
        opticalDepthCA += exp(-height / ScaleHeight) * ds;

        time += ds;
    }
    return true;
}

void main() {
    //compute ray direction
    vec2 uv = gl_FragCoord.xy;
    float scale = 2.0 * tan(CameraFov*0.5) / WindowSize.y;
    float x_offset = (uv.x - WindowSize.x * 0.5) * scale;
    float y_offset = (uv.y - WindowSize.y * 0.5) * scale;
    vec3 ray_direction = ViewDirection + x_offset * CameraRight + y_offset * CameraUp;
    ray_direction = normalize(ray_direction);

    //sample along ray direction using ray marching
    float ray_marching_step = 0.005;

    vec3 accumelate_radiances = vec3(0.0);
    vec3 view_origin = CameraPos;
    //accumulator for the opticaldepth
    float optical_depth_PA = 0.0;
    //assume we are in the earth's playground
    //compute ray marching start and end time
    //start time is the ray entry the atmosphere
    //end time is the ray exit the atmosphere
    float t_start = 0.0;
    float t_end   = 0.0;
    bool intersect = ComputeRayIntersecT(view_origin,ray_direction,t_start,t_end,PlanetCenter,PlanetAtmosphereR);
    if(!intersect){
        //would not happen in this renderer
        oFragColor = vec4(0.f,0.f,0.f,1.f);
        return;
    }
    t_start = max(t_start,0.0);

    float time = t_start;
    //compute ray marching single step distance
//    float ds = (t_end - t_start) / float(ray_marching_steps);
    int ray_marching_steps = int((t_end - t_start) / ray_marching_step);
    float ds = ray_marching_step;

    vec3 S = LightDirection;
    vec3 sun_intensity = LightRadiance;
    vec3 scattering_coefficient = vec3(BetaR,BetaG,BetaB);
    float cos_theta = dot(LightDirection,ray_direction);
    float p = PhaseCoef * (1.0 + cos_theta * cos_theta);
    vec3 phase = vec3(p);

    for(int i = 0; i < ray_marching_steps; i++){
        //sample point
        vec3 P = view_origin + ray_direction * (time + ds *0.5);

        //compute T(CP) * T(PA) * ρ(h) * ds
        float height = distance(PlanetCenter,P) - PlanetRadius;
        if(height < 0.0){
            oFragColor = vec4(1.f,1.f,0.f,1.f);
            return;
            break;
        }
        float optical_depth_segment = exp(-height / ScaleHeight) * ds;

        optical_depth_PA += optical_depth_segment;

        float optical_depth_CP = 0.0;

        bool over_ground = LightSampling(P,S,optical_depth_CP);

        if(over_ground){
            // T(CP) * T(PA) = T(CPA) = exp{ -β(λ) [D(CP) + D(PA)]}
            vec3 transmittance = exp(-scattering_coefficient * (optical_depth_CP + optical_depth_PA));

            accumelate_radiances += transmittance * optical_depth_segment;
        }
        else{
            oFragColor = vec4(0.f,1.f,0.f,1.f);
            return;
        }

        //note float precision
        time += ds;
    }

    // I = I_S * β(λ) * γ(θ) * TotalViewSamples
    vec3 I = sun_intensity * scattering_coefficient * phase * accumelate_radiances;

    vec3 color = ACESInputMat * I;

    color = RRTAndODTFit(color);

    color = ACESOutputMat * color;

    color = clamp(color,0.0,1.0);

    color = pow(I,vec3(1.0/2.2));

    oFragColor = vec4(color,1.0);
}
