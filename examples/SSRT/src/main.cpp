//
// Created by wyz on 2021/12/13.
//
#include <demo.hpp>
#include <logger.hpp>
#include <shader_program.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <array>
#include <unordered_map>


#define MODEL_NORMAL_MAP_TEX_UNIT 12
#define MODEL_DIFFUSE_MAP_TEX_UNIT 1
#define SHADOW_DEPTH_TEX_UNIT 2
#define G_BUFFER_DIFFUSE_TEX_UNIT 3
#define G_BUFFER_POS_TEX_UNIT 4
#define G_BUFFER_NORMAL_TEX_UNIT 5
#define G_BUFFER_DEPTH_TEX_UNIT 6
#define G_BUFFER_SHADOW_TEX_UNIT 7
#define DIRECT_COLOR_TEX_UNIT 8
#define INDIRECT_COLOR_TEX_UNIT 9
#define COMPOSITE_COLOR_TEX_UNIT 10

class SSRTApplication final: public Demo{
  public:
    struct Vertex{
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 tex;
        bool operator==(const Vertex& other) const{
            return pos == other.pos && normal == other.normal && tex == other.tex;
        }
    };

  private:
    void initResource() override{
        glEnable(GL_DEPTH_TEST);
        //init light
        {
            light.radiance = glm::vec3(6.f,6.f,6.f);
            light.direction = glm::normalize(glm::vec3{0.4,-0.9f,0.2f});
        }
        //load model
        loadModel();
        createShadowResource();
        createGBufferResource();
        createScreenQuad();
        createSampler();
        //create shader
        {
            const std::string shaderPath =
                "C:/Users/wyz/projects/OpenGL-Samples/examples/SSRT/shader/";
            shadow.shader = std::make_unique<Shader>(
                (shaderPath+"shadow_v.glsl").c_str(),
                (shaderPath+"shadow_f.glsl").c_str());
            shading.g_buffer_shader = std::make_unique<Shader>(
                (shaderPath+"g_buffer_v.glsl").c_str(),
                (shaderPath+"g_buffer_f.glsl").c_str());
            shading.direct_shader = std::make_unique<Shader>(
                (shaderPath+"direct_shading_v.glsl").c_str(),
                (shaderPath+"direct_shading_f.glsl").c_str());
            shading.indirect_shader = std::make_unique<Shader>(
                (shaderPath+"indirect_shading_v.glsl").c_str(),
                (shaderPath+"indirect_shading_f.glsl").c_str());
            shading.composite_shader = std::make_unique<Shader>(
                (shaderPath+"composite_v.glsl").c_str(),
                (shaderPath+"composite_f.glsl").c_str());
            screen_quad.shader = std::make_unique<Shader>(
                (shaderPath+"screen_shading_v.glsl").c_str(),
                (shaderPath+"screen_shading_f.glsl").c_str());
        }
        camera = std::make_unique<control::FPSCamera>(glm::vec3{3.54f,0.58f,1.55f});

    }
    void loadModel();
    void createShadowResource();
    void createGBufferResource();
    void createScreenQuad();
    void createSampler();

    void  render_frame() override;



  private:
    struct Light{
        glm::vec3 direction;
        glm::vec3 radiance;
    }light;
    struct{
        GL::GLVertexArray vao;
        GL::GLBuffer vbo;
        GL::GLBuffer ebo;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        GL::GLTexture diffuse_map;
        GL::GLTexture normal_map;
        GL::GLSampler sampler;
    }model;
    struct{
        GL::GLFramebuffer shadow_frame;
        GL::GLRenderbuffer shadow_rbo;
        GL::GLTexture shadow_map;
        int width=4096;
        int height=4096;
        std::unique_ptr<Shader> shader;
    } shadow;

    struct{
        GL::GLFramebuffer shading_frame;
        GL::GLRenderbuffer shading_rbo;
        //using sampler2DRect(texture2DRect) for g-buffer texture
        struct{
            GL::GLTexture g_diffuse;
            GL::GLTexture g_pos;
            GL::GLTexture g_normal;
            GL::GLTexture g_depth;
            GL::GLTexture g_shadow;
        }g_buffer;
        GL::GLTexture direct_color;
        GL::GLTexture indirect_color;
        GL::GLTexture composite_color;
        int width = 0;
        int height = 0;
        std::unique_ptr<Shader> g_buffer_shader;
        std::unique_ptr<Shader> direct_shader;
        std::unique_ptr<Shader> indirect_shader;
        std::unique_ptr<Shader> composite_shader;
    }shading;
    struct{
        GL::GLVertexArray screen_quad_vao;
        GL::GLBuffer screen_quad_vbo;
        std::unique_ptr<Shader> shader;
    }screen_quad;
};
void SSRTApplication::render_frame()
{
    glClearColor(0.f,0.f,0.f,0.f);

    static glm::vec3 world_up = glm::vec3(0.f,1.f,0.f);
    static glm::vec3 camera_pos = glm::vec3{4.19f,1.03f,2.07f};
    static glm::vec3 camera_target = glm::vec3{2.92f,0.98f,1.55f};
    static glm::vec3 camera_right = glm::normalize(glm::cross(camera_target-camera_pos,world_up));
    static glm::vec3 camera_up = glm::normalize(glm::cross(camera_right,camera_target-camera_pos));
    glm::mat4 light_mvp;
    glm::mat4 camera_mvp;
    //shadow map
    {
        glBindFramebuffer(GL_FRAMEBUFFER, shadow.shadow_frame);
        glViewport(0,0,shadow.width,shadow.height);
        {

            glm::vec3 right = glm::normalize(glm::cross(light.direction,world_up));
            auto light_view = glm::lookAt(-light.direction * 6.f,glm::vec3(0.f),glm::cross(right,light.direction));
            auto light_proj = glm::ortho(-5.f,5.f,-5.f,5.f,0.1f,10.f);
            light_mvp = light_proj * light_view;
            shadow.shader->use();
            shadow.shader->setMat4("LightMVP",light_mvp);
        }
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(model.vao);
        glDrawElements(GL_TRIANGLES,model.indices.size(),GL_UNSIGNED_INT,nullptr);
    }
    // g-buffer
    {
        glBindFramebuffer(GL_FRAMEBUFFER,shading.shading_frame);
        glViewport(0,0,shading.width,shading.height);
        {
            auto camera_view = glm::lookAt(camera_pos,camera_target,camera_up);
            auto camera_proj = glm::perspective(glm::radians(75.f),window_w*1.f/window_h,0.1f,15.f);
            camera_mvp = camera_proj * camera_view;
            shading.g_buffer_shader->use();
            shading.g_buffer_shader->setInt("ShadowMap",SHADOW_DEPTH_TEX_UNIT);
            shading.g_buffer_shader->setInt("DiffuseMap",MODEL_DIFFUSE_MAP_TEX_UNIT);
            shading.g_buffer_shader->setInt("NormalMap",MODEL_NORMAL_MAP_TEX_UNIT);
            shading.g_buffer_shader->setMat4("LightMVP",light_mvp);
            shading.g_buffer_shader->setMat4("CameraMVP",camera_mvp);
        }
        GLenum g_buffer[] = {
            GL_COLOR_ATTACHMENT0,
            GL_COLOR_ATTACHMENT1,
            GL_COLOR_ATTACHMENT2,
            GL_COLOR_ATTACHMENT3,
            GL_COLOR_ATTACHMENT4
        };
        glDrawBuffers(5,g_buffer);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(model.vao);
        glDrawElements(GL_TRIANGLES,model.indices.size(),GL_UNSIGNED_INT,nullptr);
        glFinish();
    }
    //*generate depth mipmap
    {

    }
    //direct light shading
    {
        glBindFramebuffer(GL_FRAMEBUFFER,shading.shading_frame);
        {
            shading.direct_shader->use();
            shading.direct_shader->setMat4("CameraMVP",camera_mvp);
            shading.direct_shader->setVec3("LightDir",light.direction);
            shading.direct_shader->setVec3("CameraPos",camera_pos);
            shading.direct_shader->setVec3("LightRadiance",light.radiance);
            shading.direct_shader->setVec3("ks",glm::vec3{0.8f,0.8f,0.8f});
        }
        glDrawBuffer(GL_COLOR_ATTACHMENT5);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(model.vao);
        glDrawElements(GL_TRIANGLES,model.indices.size(),GL_UNSIGNED_INT,nullptr);
    }
    //indirect light shading
    {
        glBindFramebuffer(GL_FRAMEBUFFER,shading.shading_frame);
        {
            shading.indirect_shader->use();
            shading.indirect_shader->setMat4("CameraMVP",camera_mvp);
            shading.indirect_shader->setVec3("LightDir",light.direction);
            shading.indirect_shader->setVec3("CameraPos",camera_pos);
            shading.indirect_shader->setVec3("LightRadiance",light.radiance);
            shading.indirect_shader->setIVec2("window",glm::ivec2{shading.width,shading.height});
        }
        glDrawBuffer(GL_COLOR_ATTACHMENT6);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(screen_quad.screen_quad_vao);
        glDrawArrays(GL_TRIANGLES,0,6);
    }
    //blend
    {
        glBindFramebuffer(GL_FRAMEBUFFER,shading.shading_frame);
        {
            shading.composite_shader->use();
        }
        glDrawBuffer(GL_COLOR_ATTACHMENT7);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(screen_quad.screen_quad_vao);
        glDrawArrays(GL_TRIANGLES,0,6);
    }
    //composite result
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        {
            screen_quad.shader->use();
            screen_quad.shader->setInt("DrawModel",7);
        }
        glBindVertexArray(screen_quad.screen_quad_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    GL_CHECK
}
namespace std{
template <> struct hash<SSRTApplication::Vertex>{
    size_t operator()(const SSRTApplication::Vertex& vertex) const{
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1)  ^ (hash<glm::vec2>()(vertex.tex)<<1);
    }
};
}
void SSRTApplication::loadModel()
{
    LOG_INFO("{}",__FUNCTION__ );
    const std::string assetPath = "C:/Users/wyz/projects/OpenGL-Samples/data/";
    const std::string modelPath = "cave/cave.obj";
    const std::string normalPath = "cave/siEoZ_2K_Normal_LOD0.jpg";
    const std::string albedoPath = "cave/siEoZ_2K_Albedo.jpg";
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn,err;

    if(!tinyobj::LoadObj(&attrib,&shapes,&materials,&warn,&err,(assetPath+modelPath).c_str())){
        throw std::runtime_error(warn+err);
    }
    std::unordered_map<Vertex,uint32_t> uniqueVertices{};
    for(const auto& shape:shapes){
        for(const auto& index:shape.mesh.indices){
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            vertex.normal = {
                attrib.vertices[3 * index.normal_index + 0],
                attrib.vertices[3 * index.normal_index + 1],
                attrib.vertices[3 * index.normal_index +2]
            };
            vertex.tex = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.f - attrib.texcoords[2 * index.texcoord_index + 1]
            };


            if(uniqueVertices.count(vertex) == 0){
                uniqueVertices[vertex] = static_cast<uint32_t>(model.vertices.size());
                model.vertices.push_back(vertex);
            }
            model.indices.push_back(uniqueVertices[vertex]);
        }
    }

    model.vao = gl->CreateVertexArray();
    glBindVertexArray(model.vao);
    model.vbo = gl->CreateBuffer();
    model.ebo = gl->CreateBuffer();
    glBindBuffer(GL_ARRAY_BUFFER,model.vbo);
    glBufferData(GL_ARRAY_BUFFER,model.vertices.size()*sizeof(Vertex),model.vertices.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,model.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,model.indices.size()*sizeof(uint32_t),model.indices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    LOG_INFO("load vertex size: {}, index size: {}",model.vertices.size(),model.indices.size());
    //load texture
    int texWidth,texHeight,texChannels;
    auto pixels = stbi_load((assetPath+normalPath).c_str(),&texWidth,&texHeight,&texChannels,STBI_rgb);
    assert(texChannels == 3 && pixels);
    model.normal_map = gl->CreateTexture(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,model.normal_map);
    glBindTextureUnit(MODEL_NORMAL_MAP_TEX_UNIT,model.normal_map);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,texWidth,texHeight,0,GL_RGB,GL_UNSIGNED_BYTE,pixels);
    LOG_INFO("normal map shape: {} {} {}, tex handle: {}",texWidth,texHeight,texChannels,model.normal_map);

    pixels = stbi_load((assetPath+albedoPath).c_str(),&texWidth,&texHeight,&texChannels,STBI_rgb);
    assert(texChannels == 3 && pixels);
    model.diffuse_map = gl->CreateTexture(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,model.diffuse_map);
    glBindTextureUnit(MODEL_DIFFUSE_MAP_TEX_UNIT,model.diffuse_map);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,texWidth,texHeight,0,GL_RGB,GL_UNSIGNED_BYTE,pixels);
    LOG_INFO("diffuse map shape: {} {} {}, tex handle: {}",texWidth,texHeight,texChannels,model.diffuse_map);


    //    glFinish();
    GL_CHECK
}
void SSRTApplication::createShadowResource()
{
    LOG_INFO("{}",__FUNCTION__ );
    shadow.shadow_frame = gl->CreateFramebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER,shadow.shadow_frame);
    LOG_INFO("shadow framebuffer handle: {}",shadow.shadow_frame);
    GL_CHECK

    shadow.shadow_rbo = gl->CreateRenderbuffer();
    glBindRenderbuffer(GL_RENDERBUFFER,shadow.shadow_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH24_STENCIL8,shadow.width,shadow.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,shadow.shadow_rbo);
    LOG_INFO("shadow renderbuffer handle: {}",shadow.shadow_rbo);
    GL_CHECK

    shadow.shadow_map = gl->CreateTexture(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,shadow.shadow_map);
    glBindTextureUnit(SHADOW_DEPTH_TEX_UNIT,shadow.shadow_map);
    glTextureStorage2D(shadow.shadow_map,1,GL_R32F,shadow.width,shadow.height);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,shadow.shadow_map,0);
    LOG_INFO("shadow map tex handle: {}",shadow.shadow_map);
    GL_CHECK

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error("Framebuffer object is not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GL_CHECK
}
void SSRTApplication::createGBufferResource()
{
    LOG_INFO("{}",__FUNCTION__ );

    shading.width = this->window_w;
    shading.height =this->window_h;

    shading.shading_frame = gl->CreateFramebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER,shading.shading_frame);

    shading.shading_rbo = gl->CreateRenderbuffer();
    glBindRenderbuffer(GL_RENDERBUFFER,shading.shading_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH24_STENCIL8,shading.width,shading.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,shading.shading_rbo);
    GL_CHECK

    shading.g_buffer.g_diffuse = gl->CreateTexture(GL_TEXTURE_2D);
    glTextureStorage2D(shading.g_buffer.g_diffuse,1,GL_RGBA32F,shading.width,shading.height);
    glBindTexture(GL_TEXTURE_2D,shading.g_buffer.g_diffuse);
    glBindTextureUnit(G_BUFFER_DIFFUSE_TEX_UNIT,shading.g_buffer.g_diffuse);
    glBindImageTexture(0,shading.g_buffer.g_diffuse,0,GL_FALSE,0,GL_READ_WRITE,GL_RGBA32F);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,shading.g_buffer.g_diffuse,0);

    shading.g_buffer.g_pos = gl->CreateTexture(GL_TEXTURE_2D);
    glTextureStorage2D(shading.g_buffer.g_pos,1,GL_RGBA32F,shading.width,shading.height);
    glBindTexture(GL_TEXTURE_2D,shading.g_buffer.g_pos);
    glBindTextureUnit(G_BUFFER_POS_TEX_UNIT,shading.g_buffer.g_pos);
    glBindImageTexture(1,shading.g_buffer.g_pos,0,GL_FALSE,0,GL_READ_WRITE,GL_RGBA32F);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT1,GL_TEXTURE_2D,shading.g_buffer.g_pos,0);

    shading.g_buffer.g_normal = gl->CreateTexture(GL_TEXTURE_2D);
    glTextureStorage2D(shading.g_buffer.g_normal,1,GL_RGBA32F,shading.width,shading.height);
    glBindTexture(GL_TEXTURE_2D,shading.g_buffer.g_normal);
    glBindTextureUnit(G_BUFFER_NORMAL_TEX_UNIT,shading.g_buffer.g_normal);
    glBindImageTexture(2,shading.g_buffer.g_normal,0,GL_FALSE,0,GL_READ_WRITE,GL_RGBA32F);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT2,GL_TEXTURE_2D,shading.g_buffer.g_normal,0);

    shading.g_buffer.g_depth = gl->CreateTexture(GL_TEXTURE_2D);
    glTextureStorage2D(shading.g_buffer.g_depth,1,GL_R32F,shading.width,shading.height);
    glBindTexture(GL_TEXTURE_2D,shading.g_buffer.g_depth);
    glBindTextureUnit(G_BUFFER_DEPTH_TEX_UNIT,shading.g_buffer.g_depth);
    glBindImageTexture(3,shading.g_buffer.g_depth,0,GL_FALSE,0,GL_READ_WRITE,GL_R32F);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT3,GL_TEXTURE_2D,shading.g_buffer.g_depth,0);

    shading.g_buffer.g_shadow = gl->CreateTexture(GL_TEXTURE_2D);
    glTextureStorage2D(shading.g_buffer.g_shadow,1,GL_R32F,shading.width,shading.height);
    glBindTexture(GL_TEXTURE_2D,shading.g_buffer.g_shadow);
    glBindTextureUnit(G_BUFFER_SHADOW_TEX_UNIT,shading.g_buffer.g_shadow);
    glBindImageTexture(4,shading.g_buffer.g_shadow,0,GL_FALSE,0,GL_READ_WRITE,GL_R32F);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT4,GL_TEXTURE_2D,shading.g_buffer.g_shadow,0);

    shading.direct_color = gl->CreateTexture(GL_TEXTURE_2D);
    glTextureStorage2D(shading.direct_color,1,GL_RGBA8,shading.width,shading.height);
    glBindTexture(GL_TEXTURE_2D,shading.direct_color);
    glBindTextureUnit(DIRECT_COLOR_TEX_UNIT,shading.direct_color);
    glBindImageTexture(5,shading.direct_color,0,GL_FALSE,0,GL_READ_WRITE,GL_RGBA8);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT5,GL_TEXTURE_2D,shading.direct_color,0);

    shading.indirect_color = gl->CreateTexture(GL_TEXTURE_2D);
    glTextureStorage2D(shading.indirect_color,1,GL_RGBA8,shading.width,shading.height);
    glBindTexture(GL_TEXTURE_2D,shading.indirect_color);
    glBindTextureUnit(INDIRECT_COLOR_TEX_UNIT,shading.indirect_color);
    glBindImageTexture(6,shading.indirect_color,0,GL_FALSE,0,GL_READ_WRITE,GL_RGBA8);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT6,GL_TEXTURE_2D,shading.indirect_color,0);

    shading.composite_color = gl->CreateTexture(GL_TEXTURE_2D);
    glTextureStorage2D(shading.composite_color,1,GL_RGBA8,shading.width,shading.height);
    glBindTexture(GL_TEXTURE_2D,shading.composite_color);
    glBindTextureUnit(COMPOSITE_COLOR_TEX_UNIT,shading.composite_color);
    glBindImageTexture(7,shading.composite_color,0,GL_FALSE,0,GL_READ_WRITE,GL_RGBA8);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT7,GL_TEXTURE_2D,shading.composite_color,0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error("Framebuffer object is not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GL_CHECK
}
void SSRTApplication::createScreenQuad()
{
    LOG_INFO("{}",__FUNCTION__ );
    std::array<float, 12> screen_quad_vertices = {
        -1.f,1.f,
        1.f,-1.f,
        1.f,1.f,
        -1.f,1.f,
        -1.f,-1.f,
        1.f,-1.f
    };
    screen_quad.screen_quad_vao = gl->CreateVertexArray();
    GL_CHECK
    glBindVertexArray(screen_quad.screen_quad_vao);
    GL_CHECK
    screen_quad.screen_quad_vbo = gl->CreateBuffer();
    glBindBuffer(GL_ARRAY_BUFFER,screen_quad.screen_quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screen_quad_vertices), screen_quad_vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    GL_CHECK
}
void SSRTApplication::createSampler()
{
    LOG_INFO("{}",__FUNCTION__ );
    model.sampler = gl->CreateSampler();
    glSamplerParameterf(model.sampler,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glSamplerParameterf(model.sampler,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glSamplerParameterf(model.sampler,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
    glSamplerParameterf(model.sampler,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);

    glBindSampler(MODEL_NORMAL_MAP_TEX_UNIT,model.sampler);
    glBindSampler(MODEL_DIFFUSE_MAP_TEX_UNIT,model.sampler);
    glBindSampler(SHADOW_DEPTH_TEX_UNIT,model.sampler);
}

int main(){
    try
    {
        SSRTApplication app;
        app.run();
    }
    catch (const std::exception& err)
    {
        std::cout<<err.what()<<std::endl;
    }
    return 0;
}