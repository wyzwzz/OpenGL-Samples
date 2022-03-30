#include <shader_program.hpp>
#include <demo.hpp>
#include <mesh.hpp>
#include <logger.hpp>
#include <array>
#include <fstream>
#include <texture.hpp>

#include <imgui.h>

const float PI = 3.14159265359;

const std::string AssetPath = "C:/Users/wyz/projects/OpenGL-Samples/data/";
const std::string ShaderPath = "C:/Users/wyz/projects/OpenGL-Samples/examples/Kulla-ContyBRDF/shader/";

const int LutResolution = 128;

float RadicalInverse_VdC(uint32_t bits){
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}


vec2f Hammersley(uint32_t i, uint32_t N){
    return vec2f(static_cast<float>(i)/static_cast<float>(N),RadicalInverse_VdC(i));
}


vec3f ImportanceSampleGGX(vec2f Xi, vec3f N, float roughness)
{
	float a = roughness*roughness;
	
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	// from spherical coordinates to cartesian coordinates - halfway vector
	vec3f H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	// from tangent-space H vector to world-space sample vector
	vec3f up          = abs(N.z) < 0.999 ? vec3f(0.0, 0.0, 1.0) : vec3f(1.0, 0.0, 0.0);
	vec3f tangent   = normalize(cross(up, N));
	vec3f bitangent = cross(N, tangent);
	
	vec3f sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    // note that we use a different k for IBL
    // 与直接光渲染的k计算不同
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float roughness, float NdotV, float NdotL)
{
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
float IntegrateBRDF(vec3f V, float roughness){
    const int sample_count = 1024;
    vec3f N = vec3f(0.f,0.f,1.f);
    float A = 0.f;
    for(int i = 0; i < sample_count; i++){
        vec2f Xi = Hammersley(i , sample_count);
        vec3f H = ImportanceSampleGGX(Xi,N,roughness);
        vec3f L = glm::normalize(H * 2.f * dot(V, H) - V);

        float NoL = std::max(L.z,0.f);
        float NoH = std::max(H.z,0.f);
        float VoH = std::max(dot(V, H), 0.f);
        float NoV = std::max(dot(N, V), 0.f);

        auto G = GeometrySmith(roughness, NoV, NoL);
        auto weight = VoH * G / (NoV * NoH);
        A += weight;
    }
    return A / (float)sample_count;
}

auto GenerateEmiuInCPU(){
    Texture2D<float> Emiu({LutResolution,LutResolution},0.f);
    float step = 1.f / LutResolution;
    for(int i = 0; i < LutResolution; i++){
        for(int j = 0; j < LutResolution; j++){
            float roughness = step * (static_cast<float>(i) + 0.5f);
            float NdotV = step * (static_cast<float>(j) + 0.5f);
            // 只考虑光线与N的夹角 而不考虑光线投影在反射平面内的方向 
            vec3f V = vec3f(std::sqrt(1.f - NdotV*NdotV),0.f,NdotV);
            Emiu(i,j) = IntegrateBRDF(V,roughness);
        }
    }
    return Emiu;
}

auto GenerateEavgInCPU(const Texture2D<float>& Emiu){
    Texture1D<float> Eavg({LutResolution},0.f);
    float step = 1.f / LutResolution;
    for(int i = 0; i < LutResolution; i++){
        float sum = 0.f;
        for(int j = 0; j < LutResolution; j++){
            float miu = (static_cast<float>(i) + 0.5f) / static_cast<float>(LutResolution);
            sum += miu * Emiu(i, j) * step;
        }
        Eavg(i) = 2.f * sum; 
    }
    return Eavg;
}



class KullaContyBRDFAPPlication final : public Demo{
    void initResource() override;
    void process_input() override;
    void render_frame() override;
    void render_imgui() override;

    void createLUT(){
        createEmiu();
        createEavg();
    }

    void createEmiu(){
        lut.Emiu_data = GenerateEmiuInCPU();
        lut.Emiu_tex = gl->CreateTexture(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D,lut.Emiu_tex);
        glTexImage2D(GL_TEXTURE_2D,0,GL_R32F,
        lut.Emiu_data.GetWidth(),lut.Emiu_data.GetHeight(),0,
        GL_RED,GL_FLOAT,lut.Emiu_data.RawData());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTextureUnit(lut.EmiuTexUnit,lut.Emiu_tex);
        GL_CHECK
    }
    void createEavg(){
        lut.Eavg_data = GenerateEavgInCPU(lut.Emiu_data);
        lut.Eavg_tex = gl->CreateTexture(GL_TEXTURE_1D);
        glBindTexture(GL_TEXTURE_1D,lut.Eavg_tex);
        glTexImage1D(GL_TEXTURE_1D,0,GL_R32F,
        lut.Eavg_data.GetLength(),0,GL_RED,GL_FLOAT,lut.Eavg_data.RawData());
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTextureUnit(lut.EavgTexUint,lut.Eavg_tex);
        GL_CHECK
    }

    void loadDrawModel(){
        auto triangles = load_triangles_from_obj(AssetPath+"ball/ball.obj");
        LOG_INFO("triangles count: {}",triangles.size());
        draw_model.triangle_count = triangles.size();
        draw_model.vao = gl->CreateVertexArray();
        glBindVertexArray(draw_model.vao);
        draw_model.vbo = gl->CreateBuffer();
        glBindBuffer(GL_ARRAY_BUFFER,draw_model.vbo);
        glBufferData(GL_ARRAY_BUFFER,sizeof(triangle_t)*triangles.size(),triangles.data(),GL_STATIC_DRAW);
        assert(sizeof(triangle_t)==96 && sizeof(vertex_t)==32);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(0));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);

        draw_model.albedo = {0.827f,0.792f,0.678f};
        draw_model.metallic = 0.9f;
        draw_model.roughness = 0.7f;
    }

    void createShader(){
        kc_pbr_shader = std::make_unique<Shader>(
            (ShaderPath+"kulla_conty_pbr_v.glsl").c_str(),
            (ShaderPath+"kulla_conty_pbr_f.glsl").c_str()
        );
        kc_pbr_shader->use();
        kc_pbr_shader->setInt("uBRDFLut",lut.EmiuTexUnit);
        kc_pbr_shader->setInt("uEavgLut",lut.EavgTexUint);
    }
    
    struct LUT{
        Texture2D<float> Emiu_data;
        Texture1D<float> Eavg_data;
        GL::GLTexture Emiu_tex;
        GL::GLTexture Eavg_tex;
        static constexpr uint32_t EmiuTexUnit = 0;
        static constexpr uint32_t EavgTexUint = 1;
    }lut;

    struct{
        vec3f position;
        vec3f radiance;
    }light;
    
    struct{
        GL::GLVertexArray vao;
        GL::GLBuffer vbo;
        vec3f albedo;
        float metallic;
        float roughness;
        uint32_t triangle_count{0};
    }draw_model;

    std::unique_ptr<Shader> kc_pbr_shader;
    bool kc{false};
};

void KullaContyBRDFAPPlication::initResource(){
    glEnable(GL_DEPTH_TEST);

    createLUT();
    loadDrawModel();
    createShader();

    light.position = {0.f,0.f,8.f};
    light.radiance = {120.f,110.f,10.f};

    

    camera = std::make_unique<control::FPSCamera>(glm::vec3{0.f,5.f,7.f});
}

void KullaContyBRDFAPPlication::process_input(){
    static auto t = glfwGetTime();
    auto cur_t = glfwGetTime();
    auto delta_t = cur_t - t;
    t = cur_t;

    delta_t *= 0.2;

    if(glfwGetKey(gl->GetWindow(),GLFW_KEY_W))
    camera->processKeyEvent(control::CameraDefinedKey::Forward,delta_t);
    if(glfwGetKey(gl->GetWindow(),GLFW_KEY_S))
    camera->processKeyEvent(control::CameraDefinedKey::Backward,delta_t);
    if(glfwGetKey(gl->GetWindow(),GLFW_KEY_A))
    camera->processKeyEvent(control::CameraDefinedKey::Left,delta_t);
    if(glfwGetKey(gl->GetWindow(),GLFW_KEY_D))
    camera->processKeyEvent(control::CameraDefinedKey::Right,delta_t);
    if(glfwGetKey(gl->GetWindow(),GLFW_KEY_Q))
    camera->processKeyEvent(control::CameraDefinedKey::Up,delta_t);
    if(glfwGetKey(gl->GetWindow(),GLFW_KEY_E))
    camera->processKeyEvent(control::CameraDefinedKey::Bottom,delta_t);
}

void KullaContyBRDFAPPlication::render_frame(){
    glClearColor(0.f,0.f,0.f,0.f);
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

    kc_pbr_shader->use();
    kc_pbr_shader->setMat4("model",glm::mat4(1.f));
    kc_pbr_shader->setMat4("view",camera->getViewMatrix());
    kc_pbr_shader->setMat4("projection",
    glm::perspective(
        glm::radians(camera->getZoom()),(float)gl->Width()/gl->Height(),0.01f,100.f));

    kc_pbr_shader->setVec3("uAlbode",draw_model.albedo);
    kc_pbr_shader->setFloat("uMetallic",draw_model.metallic);
    kc_pbr_shader->setFloat("uRoughness",draw_model.roughness);
    kc_pbr_shader->setVec3("uLightPos",light.position);
    kc_pbr_shader->setVec3("uLightRadiance",light.radiance);
    kc_pbr_shader->setVec3("uCameraPos",camera->getCameraPos());
    kc_pbr_shader->setBool("uKC",kc);

    glBindVertexArray(draw_model.vao);
    glDrawArrays(GL_TRIANGLES,0,draw_model.triangle_count*3);

    GL_CHECK
}

void KullaContyBRDFAPPlication::render_imgui(){
    ImGui::Text("Kulla-ContyBRDF");
    ImGui::SliderFloat3("albedo",&draw_model.albedo.x,0.f,1.f);
    ImGui::SliderFloat("metallic",&draw_model.metallic,0.f,1.f);
    ImGui::SliderFloat("roughness",&draw_model.roughness,0.f,1.f);
    ImGui::InputFloat3("light position",&light.position.x,2);
    ImGui::SliderFloat3("light radiance",&light.radiance.x,0.f,1000.f);
    ImGui::Checkbox("KullaConty",&kc);
}

int main(){
    try{
        KullaContyBRDFAPPlication app;
        app.run();
    }
    catch(const std::exception& err){
        LOG_ERROR("{}",err.what());
    }
    return 0;
}