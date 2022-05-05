#version 460 core
layout(location = 0) out vec4 oFragColor;
uniform sampler2D blurTexture;
uniform vec2 resolution;
uniform int curLod;
const float weight[5] = float[] (0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);
uniform bool horizontal;
void main() {
    vec2 uv = gl_FragCoord.xy / resolution;
    vec4 color = textureLod(blurTexture,uv,curLod) * weight[0];
    vec2 textureSize = 1.0 / resolution;
    if(horizontal){
        for(int i = 1;i<5;i++){
            color += textureLod(blurTexture,uv + vec2(textureSize.x * i,0.0),curLod) * weight[i];
            color += textureLod(blurTexture,uv - vec2(textureSize.x * i,0.0),curLod) * weight[i];
        }
    }
    else{
        for(int i = 1;i<5;i++){
            color += textureLod(blurTexture,uv + vec2(0.0,textureSize.y * i),curLod) * weight[i];
            color += textureLod(blurTexture,uv - vec2(0.0,textureSize.y * i),curLod) * weight[i];
        }
    }
    color.a = 1.0;
    oFragColor = color;
}
