#version 460 core
layout(location = 0) in vec3 inWorldPos;
layout(location = 0) out vec4 outFragColor;

uniform samplerCube enviromentMap;

const float PI = 3.14159265359;

void main(){
    //根据法线方向 采样半球的环境光进行卷积
    //对半球进行积分时采用黎曼和形式 
    //使用theta和pi两个微元代替单位立体角 积分范围从0-2*PI和0-PI/2
    //根据theta和pi得到的方向需要转换到世界坐标，即旋转到N 但是其它两个维度不做限制 取特殊的up
    vec3 N = normalize(inWorldPos);
    
    vec3 irradiance = vec3(0.f);

    vec3 up = vec3(0.f,1.f,0.f);//other is ok
    vec3 right = normalize(cross(up,N));
    up = normalize(cross(N,right));

    float sample_delta = 0.025f;
    float nr_samples = 0.0;
    for(float phi = 0.f; phi < 2.f * PI; phi += sample_delta){
        for(float theta = 0.f; theta < 0.5f * PI; theta += sample_delta){
            //direction in tangent space
            vec3 tangent_sample = vec3(sin(theta)*cos(phi),sin(theta)*sin(phi),cos(theta));
            //tangent space to world
            vec3 sample_vec = tangent_sample.x * right + tangent_sample.y * up + tangent_sample.z * N;

            irradiance += texture(enviromentMap,sample_vec).rgb * cos(theta) * sin(theta);
            nr_samples ++;
        }
    }
    irradiance = PI * irradiance * (1.f / nr_samples);
    outFragColor = vec4(irradiance,1.f);
}