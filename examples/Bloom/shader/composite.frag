#version 460 core
layout(location = 0) out vec4 oFragColor;
uniform sampler2D blurTexture;
uniform sampler2D renderTexture;
uniform int mode;
uniform vec2 resolution;
uniform float exposure;
void main() {
    vec2 uv = gl_FragCoord.xy / resolution;
    vec4 bloom_color = texture(blurTexture,uv);
    vec4 render_color = texture(renderTexture,uv);
    if(mode == 0){
        oFragColor = render_color;
    }
    else if(mode == 1){
        oFragColor = bloom_color;
    }
    else if(mode == 2){
        vec3 color = (render_color + bloom_color).rgb;
        color.rgb = vec3(1.0) - exp(-color * exposure);
        color = pow(color,vec3(1.0/2.2));
        oFragColor.rgb = color;
        oFragColor.a = 1.0;
    }
}
