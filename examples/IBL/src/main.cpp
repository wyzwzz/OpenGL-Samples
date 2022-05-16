//
// Created by wyz on 2021/12/20.
//
#include <iostream>
#include <vector>
#include <array>
#include <demo.hpp>
#include <logger.hpp>
#include <shader_program.hpp>

#include <imgui.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

static const std::string ShaderPath = "C:/Users/wyz/projects/OpenGL-Samples/examples/IBL/shader/";
static const std::string TexturePath = "C:/Users/wyz/projects/OpenGL-Samples/data/textures/IBL/";
static const float PI = 3.14159265359f;
struct Sphere{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uv;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;
};
static Sphere MakeSphere(){
    static constexpr uint32_t X_SEGMENTS = 64;
    static constexpr uint32_t Y_SEGMENTS = 64;
    static constexpr float PI = 3.14159265359f;
    
    Sphere sphere;
    
    for(uint32_t y = 0; y <= Y_SEGMENTS; y++){
        for(uint32_t x = 0; x <= X_SEGMENTS; x++){
            float x_segment = static_cast<float>(x) / X_SEGMENTS;
            float y_segment = static_cast<float>(y) / Y_SEGMENTS;
            float x_pos = std::cos(x_segment * 2.f * PI) * std::sin(y_segment * PI);
            float y_pos = std::cos(y_segment * PI);
            float z_pos = std::sin(x_segment * 2.f * PI) * std::sin(y_segment * PI);

            sphere.positions.emplace_back(x_pos,y_pos,z_pos);
            sphere.uv.emplace_back(x_segment,y_segment);
            sphere.normals.emplace_back(x_pos,y_pos,z_pos);
        }
    }
    //use GL_TRIANGLE_STRIP
    bool odd_row = false;
    for(uint32_t y = 0; y < Y_SEGMENTS; y++){
        if(!odd_row){
            for(uint32_t x = 0; x <= X_SEGMENTS; x++){
                sphere.indices.emplace_back( y    * (X_SEGMENTS + 1) + x);
                sphere.indices.emplace_back((y+1) * (X_SEGMENTS + 1) + x);
            }
        }
        else{
            for(int x = X_SEGMENTS; x>=0; x--){
                sphere.indices.emplace_back((y + 1) * (X_SEGMENTS + 1) + x);
                sphere.indices.emplace_back( y      * (X_SEGMENTS + 1) + x);
            }
        }
        odd_row = !odd_row;
    }

    return sphere;
}
static void CreateSphere(const std::shared_ptr<GL>& gl,GL::GLVertexArray& vao,GL::GLBuffer& vbo,GL::GLBuffer& ebo,int& index_count){
    auto sphere = MakeSphere();
    index_count = sphere.indices.size();
    vao = gl->CreateVertexArray();
    glBindVertexArray(vao);
    vbo = gl->CreateBuffer();
    ebo = gl->CreateBuffer();

    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    assert(sphere.positions.size()==sphere.normals.size() && sphere.normals.size() == sphere.uv.size());
    auto vertex_count = sphere.positions.size();
    glBufferData(GL_ARRAY_BUFFER,vertex_count*sizeof(float)*8,nullptr,GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER,0,vertex_count*3*sizeof(float),sphere.positions.data());
    glBufferSubData(GL_ARRAY_BUFFER,vertex_count*3*sizeof(float),vertex_count*3*sizeof(float),sphere.normals.data());
    glBufferSubData(GL_ARRAY_BUFFER,vertex_count*6*sizeof(float),vertex_count*2*sizeof(float),sphere.uv.data());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,index_count*sizeof(uint32_t),sphere.indices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)(vertex_count*3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)(vertex_count*6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

static void CreateTexture(const std::shared_ptr<GL>& gl,GL::GLTexture& tex,const std::string& path){
    int width,height,nrComponents;
    auto data = stbi_load(path.c_str(),&width,&height,&nrComponents,0);
    if(data){
        tex = gl->CreateTexture(GL_TEXTURE_2D);
        GLenum format;
        if(nrComponents == 1) format = GL_RED;
        else if(nrComponents == 3) format = GL_RGB;
        else if(nrComponents == 4) format = GL_RGBA;
        else{
            LOG_ERROR("Invalid texture nrComponets: {}",nrComponents);
            format = 0;
        }
        glBindTexture(GL_TEXTURE_2D,tex);
        glTexImage2D(GL_TEXTURE_2D,0,format,width,height,0,format,GL_UNSIGNED_BYTE,data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        //mipmap
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

        stbi_image_free(data);
        LOG_INFO("Successfully create texture: {}",path.substr(1+path.find_last_of("/")));
    }
    else{
        LOG_ERROR("Load texture {} failed!",path);
    }
    char* (*str)[2] = nullptr;
    char* s[] = {nullptr,nullptr};
    str = &s;
}

static void CreateCube(const std::shared_ptr<GL>& gl,GL::GLVertexArray& vao,GL::GLBuffer& vbo){
            float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        
        vao = gl->CreateVertexArray();
        glBindVertexArray(vao);
        vbo = gl->CreateBuffer();
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        GL_CHECK
}

static void CreateQuad(const std::shared_ptr<GL>& gl,GL::GLVertexArray& vao,GL::GLBuffer& vbo){
    static float quad_vertices[] = {
    // positions        // texture Coords
    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
    1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
    1.0f, -1.0f, 0.0f, 1.0f, 0.0f,};
    
    vao = gl->CreateVertexArray();
    vbo = gl->CreateBuffer();
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(quad_vertices),quad_vertices,GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)(3*sizeof(float)));
    glBindVertexArray(0);
    GL_CHECK
}

class DirectRenderer{
public:
    DirectRenderer(const std::shared_ptr<GL>& gl):gl(gl){}

    std::shared_ptr<GL> gl;
    struct{
        GL::GLTexture albedo;
        GL::GLTexture normal;
        GL::GLTexture metallic;
        GL::GLTexture roughness;
        GL::GLTexture ao;
    }material;
    static constexpr uint32_t albedo_tex_unit = 0;
    static constexpr uint32_t normal_tex_unit = 1;
    static constexpr uint32_t metallic_tex_unit = 2;
    static constexpr uint32_t roughness_tex_unit = 3;
    static constexpr uint32_t ao_tex_unit = 4;
    
    static constexpr uint32_t LightNum = 4;
    struct{
        glm::vec3 position;
        glm::vec3 color;
    }light[LightNum];
    struct{
        int nr_rows{7};
        int nr_colrs{7};
        float space{2.5f};
        GL::GLVertexArray vao;
        GL::GLBuffer vbo;
        GL::GLBuffer ebo;
        int index_count{0};
    }model;

    std::unique_ptr<Shader> pbr_shader;

    void init(){
        CreateSphere(gl,model.vao,model.vbo,model.ebo,model.index_count);

        pbr_shader = std::make_unique<Shader>(
            (ShaderPath+"direct_pbr_v.glsl").c_str(),
            (ShaderPath+"direct_pbr_f.glsl").c_str());

        CreateTexture(gl,material.albedo,TexturePath+"rusted_iron/albedo.png");
        CreateTexture(gl,material.normal,TexturePath+"rusted_iron/normal.png");
        CreateTexture(gl,material.metallic,TexturePath+"rusted_iron/metallic.png");
        CreateTexture(gl,material.roughness,TexturePath+"rusted_iron/roughness.png");
        CreateTexture(gl,material.ao,TexturePath+"rusted_iron/ao.png");
        pbr_shader->use();
        pbr_shader->setInt("albedoMap",albedo_tex_unit);
        pbr_shader->setInt("normalMap",normal_tex_unit);
        pbr_shader->setInt("metallicMap",metallic_tex_unit);
        pbr_shader->setInt("roughnessMap",roughness_tex_unit);
        pbr_shader->setInt("aoMap",ao_tex_unit);

        light[0] = {
            {1.f,0.f,10.f},{150.f,0.f,0.f}
        };
        light[1] = {
            {0.f,1.f,10.f},{0.f,50.f,0.f}
        };
        light[2] = {
            {-1.f,0.f,10.f},{0.f,0.f,100.f}
        };
        light[3] = {
            {0.f,-1.f,10.f},{120.f,120.f,120.f}
        };

    }
    void setCamera(const std::unique_ptr<control::FPSCamera>& camera){
        pbr_shader->use();
        pbr_shader->setMat4("view",camera->getViewMatrix());
        pbr_shader->setMat4("projection",
        glm::perspective(glm::radians(camera->getZoom()),(float)gl->Width()/gl->Height(),0.1f,100.f));
        pbr_shader->setVec3("cameraPos",camera->getCameraPos());
    }
    void draw(){
        glBindTextureUnit(albedo_tex_unit,material.albedo);
        glBindTextureUnit(normal_tex_unit,material.normal);
        glBindTextureUnit(metallic_tex_unit,material.metallic);
        glBindTextureUnit(roughness_tex_unit,material.roughness);
        glBindTextureUnit(ao_tex_unit,material.ao);

        pbr_shader->use();
        
        auto t = glfwGetTime();    
        for(int i = 0 ;i < LightNum; i++){
            t += i * PI * 0.5f;
            glm::vec3 pos = glm::vec3(2.f*cos(t),2.f*sin(t),10.f);
            pbr_shader->setVec3("lightPos["+std::to_string(i)+"]",pos);
            pbr_shader->setVec3("lightColor["+std::to_string(i)+"]",light[i].color);
        }

        for(int row = 0;row < model.nr_rows;row ++)
        {   
            for(int col = 0;col < model.nr_colrs; col++)
            {
                auto _model = glm::mat4(1.f);
                _model = glm::translate(_model,glm::vec3((float)(col-(model.nr_colrs/2))*model.space,
                (float)(row-(model.nr_rows/2))*model.space,0.f));
                pbr_shader->setMat4("model",_model);    
                glBindVertexArray(model.vao);
                glDrawElements(GL_TRIANGLE_STRIP,model.index_count,GL_UNSIGNED_INT,nullptr);
            }
        }
        GL_CHECK
    }
};

class IBLRenderer{
public:
    IBLRenderer(const std::shared_ptr<GL>& gl):gl(gl){}

    std::shared_ptr<GL> gl;
    struct CubeMap{
        GL::GLFramebuffer fbo;
        GL::GLRenderbuffer rbo;
        GL::GLTexture hdr;
        GL::GLTexture env_cube;
        static constexpr uint32_t CUBE_SIZE = 512;
        std::unique_ptr<Shader> equirectangular2cubemap_shader;
        GL::GLVertexArray vao;
        GL::GLBuffer vbo;
        std::unique_ptr<Shader> skybox_shader;
        glm::mat4 captureProj;
        std::array<glm::mat4,6> captureView;
    }cube_map;

    struct PBRMaterial{
        GL::GLTexture albeodo;
        GL::GLTexture normal;
        GL::GLTexture metallic;
        GL::GLTexture roughness;
        GL::GLTexture ao;
    };
    struct Sphere{
        GL::GLVertexArray vao;
        GL::GLBuffer vbo;
        GL::GLBuffer ebo;
        int index_count{0};
    };
    struct{
        Sphere sphere;
        PBRMaterial rusted_iron;
        PBRMaterial gold;
        PBRMaterial grass;
        PBRMaterial plastic;
        PBRMaterial wall;
    }draw_model;

    static constexpr uint32_t LightNum = 4;
    struct{
        glm::vec3 position;
        glm::vec3 color;
    }light[LightNum];
    
    struct{
        GL::GLVertexArray quad_vao;
        GL::GLBuffer quad_vbo;
    }quad;
    struct IBL{
        GL::GLTexture irradiance_map;
        static constexpr uint32_t IrradianceMapSize = 32;
        std::unique_ptr<Shader> capture_irradiance_shader;
        GL::GLTexture prefilter_map;
        static constexpr uint32_t PrefilterMapSize = 128;
        std::unique_ptr<Shader> prefilter_shader;
        static constexpr uint32_t BRDFLUTSize = 512;
        GL::GLTexture lut_map;

        std::unique_ptr<Shader> brdf_shader;
        std::unique_ptr<Shader> pbr_shader;
    }ibl;

    static constexpr int albedo_tex_unit = 0;
    static constexpr int normal_tex_unit = 1;
    static constexpr int metallic_tex_unit = 2;
    static constexpr int roughness_tex_unit = 3;
    static constexpr int ao_tex_unit = 4;
    static constexpr int irradiance_tex_unit = 5;
    static constexpr int prefilter_tex_unit = 6;
    static constexpr int lut_tex_unit = 7;

    void createHDRTexture(const std::string& path){
        stbi_set_flip_vertically_on_load(true);
        int width, height, nrComponents;
        float* data = stbi_loadf(path.c_str(),&width,&height,&nrComponents,0);
        if(data){
            cube_map.hdr = gl->CreateTexture(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D,cube_map.hdr);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGB16F,width,height,0,GL_RGB,GL_FLOAT,data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);

            LOG_INFO("successfully create hdr texture: {}",path.substr(1+path.find_last_of('/')));
        }
        else{
            LOG_ERROR("Load hdr image failed: {}",path);
        }
    }
    void captureEnvCubeMap(){
        cube_map.captureProj = glm::perspective(glm::radians(90.f),1.f,0.1f,10.f);
        cube_map.captureView = {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };
        cube_map.equirectangular2cubemap_shader->use();
        cube_map.equirectangular2cubemap_shader->setInt("equirectangularMap",0);
        cube_map.equirectangular2cubemap_shader->setMat4("projection",cube_map.captureProj);
        glBindTextureUnit(0,cube_map.hdr);
        glBindTexture(GL_TEXTURE_CUBE_MAP,cube_map.env_cube);
        GL_EXPR(glBindFramebuffer(GL_FRAMEBUFFER,cube_map.fbo));
        glViewport(0,0,cube_map.CUBE_SIZE,cube_map.CUBE_SIZE);
        glBindVertexArray(cube_map.vao);
        for(uint32_t i = 0;i < 6; i++){
            cube_map.equirectangular2cubemap_shader->setMat4("view",cube_map.captureView[i]);
            GL_EXPR(glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,cube_map.env_cube,0));           
            GL_EXPR(glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT));
            GL_EXPR(glDrawArrays(GL_TRIANGLES,0,36));
        }
        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        glViewport(0,0,gl->Width(),gl->Height());

        //create cube map
        glBindTexture(GL_TEXTURE_CUBE_MAP,cube_map.env_cube);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        GL_CHECK
    }
    void createEnvCubeMap(){
        CreateCube(gl,cube_map.vao,cube_map.vbo);
        cube_map.fbo = gl->CreateFramebuffer();
        cube_map.rbo = gl->CreateRenderbuffer();
        glBindFramebuffer(GL_FRAMEBUFFER,cube_map.fbo);
        glBindRenderbuffer(GL_RENDERBUFFER,cube_map.rbo);
        glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH24_STENCIL8,CubeMap::CUBE_SIZE,CubeMap::CUBE_SIZE);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,cube_map.rbo);
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){
            throw std::runtime_error("Framebuffer object is not complete!");
        }
        GL_CHECK

        createHDRTexture(TexturePath+"hdr/newport_loft.hdr");

        cube_map.env_cube = gl->CreateTexture(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP,cube_map.env_cube);
        for(uint32_t i = 0;i < 6; i++){
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,0,GL_RGB16F,CubeMap::CUBE_SIZE,CubeMap::CUBE_SIZE,0,GL_RGB,GL_FLOAT,nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        GL_CHECK

        cube_map.equirectangular2cubemap_shader = std::make_unique<Shader>(
            (ShaderPath+"cube_map_v.glsl").c_str(),
            (ShaderPath+"equirectangular2cubemap_f.glsl").c_str()
        );

        captureEnvCubeMap();

        cube_map.skybox_shader = std::make_unique<Shader>(
            (ShaderPath+"background_v.glsl").c_str(),
            (ShaderPath+"background_f.glsl").c_str()
        );
        cube_map.skybox_shader->use();
        cube_map.skybox_shader->setInt("environmentMap",0);
    }
    void solveIrradianceMap(){
        glBindFramebuffer(GL_FRAMEBUFFER,cube_map.fbo);
        GL_EXPR(glNamedRenderbufferStorage(cube_map.rbo,GL_DEPTH24_STENCIL8,IBL::IrradianceMapSize,IBL::IrradianceMapSize));
//        glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,cube_map.rbo);
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){
            throw std::runtime_error("IrradianceMap framebuffer object is not complete!");
        }
        GL_CHECK

        ibl.capture_irradiance_shader->use();
        ibl.capture_irradiance_shader->setInt("environmentMap",0);
        ibl.capture_irradiance_shader->setMat4("projection",cube_map.captureProj);
        glBindTextureUnit(0,cube_map.env_cube);
        glBindTexture(GL_TEXTURE_CUBE_MAP,cube_map.env_cube);
        glViewport(0,0,ibl.IrradianceMapSize,ibl.IrradianceMapSize);
        glBindVertexArray(cube_map.vao);
        for(uint32_t i = 0; i < 6; i++){
            ibl.capture_irradiance_shader->setMat4("view",cube_map.captureView[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,ibl.irradiance_map,0);
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES,0,36);
        }
        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        glViewport(0,0,gl->Width(),gl->Height());
        GL_CHECK
    }
    void createDiffuseMap(){
        ibl.capture_irradiance_shader = std::make_unique<Shader>(
            (ShaderPath+"cube_map_v.glsl").c_str(),
            (ShaderPath+"irradiance_convolution_f.glsl").c_str()
        );
        ibl.irradiance_map = gl->CreateTexture(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP,ibl.irradiance_map);
        for(uint32_t i = 0; i < 6; i++){
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,0,GL_RGB16F,IBL::IrradianceMapSize,IBL::IrradianceMapSize,
            0,GL_RGB,GL_FLOAT,nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_CHECK

        solveIrradianceMap();

    }
    void calcPrefilterMap(){
        glBindFramebuffer(GL_FRAMEBUFFER,cube_map.fbo);
        ibl.prefilter_shader->use();
        ibl.prefilter_shader->setInt("environmentMap",0);
        ibl.prefilter_shader->setMat4("projection",cube_map.captureProj);
        glBindTextureUnit(0,cube_map.env_cube);
        glBindTexture(GL_TEXTURE_CUBE_MAP,cube_map.env_cube);
        uint32_t max_mip_levels = 5;
        for(uint32_t mip = 0; mip < max_mip_levels; mip++){
            uint32_t mip_width = ibl.PrefilterMapSize * std::pow(0.5,mip);
            uint32_t mip_height = ibl.PrefilterMapSize * std::pow(0.5,mip);
            glBindRenderbuffer(GL_RENDERBUFFER,cube_map.rbo);
            glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH24_STENCIL8,mip_width,mip_height);
            glViewport(0,0,mip_width,mip_height);

            float roughness = static_cast<float>(mip) / (max_mip_levels - 1);
            ibl.prefilter_shader->setFloat("roughness",roughness);
            for(uint32_t i = 0; i < 6; i++){
                ibl.prefilter_shader->setMat4("view",cube_map.captureView[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,ibl.prefilter_map,mip);
                glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
                glBindVertexArray(cube_map.vao);
                glDrawArrays(GL_TRIANGLES,0,36);
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER,0);
    }
    void generateBRDFLUTMap(){
        glBindFramebuffer(GL_FRAMEBUFFER,cube_map.fbo);
        glBindRenderbuffer(GL_RENDERBUFFER,cube_map.rbo);
        glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH24_STENCIL8,ibl.BRDFLUTSize,ibl.BRDFLUTSize);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,ibl.lut_map,0);
        glViewport(0,0,ibl.BRDFLUTSize,ibl.BRDFLUTSize);
        
        ibl.brdf_shader->use();
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(quad.quad_vao);
        glDrawArrays(GL_TRIANGLE_STRIP,0,4);
        
        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        GL_CHECK
    }
    void createSpecularMap(){
        ibl.prefilter_map = gl->CreateTexture(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP,ibl.prefilter_map);
        for(uint32_t i = 0; i < 6; i++){
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,0,GL_RGB16F,ibl.PrefilterMapSize,ibl.PrefilterMapSize,0,GL_RGB,GL_FLOAT,nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        GL_CHECK

        ibl.prefilter_shader = std::make_unique<Shader>(
            (ShaderPath+"cube_map_v.glsl").c_str(),
            (ShaderPath+"prefilter_f.glsl").c_str()
        );

        calcPrefilterMap();

        ibl.lut_map = gl->CreateTexture(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D,ibl.lut_map);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RG16F,ibl.BRDFLUTSize,ibl.BRDFLUTSize,0,GL_RG,GL_FLOAT,nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_CHECK

        ibl.brdf_shader = std::make_unique<Shader>(
            (ShaderPath+"brdf_v.glsl").c_str(),
            (ShaderPath+"brdf_f.glsl").c_str()
        );

        CreateQuad(gl,quad.quad_vao,quad.quad_vbo);
        generateBRDFLUTMap();
    }
    void LoadMaterial(PBRMaterial& material,const std::string& name){
        CreateTexture(gl,material.albeodo,TexturePath+name+"/albedo.png");
        CreateTexture(gl,material.normal,TexturePath+name+"/normal.png");
        CreateTexture(gl,material.metallic,TexturePath+name+"/metallic.png");
        CreateTexture(gl,material.roughness,TexturePath+name+"/roughness.png");
        CreateTexture(gl,material.ao,TexturePath+name+"/ao.png");
    }
    void createDrawModel(){
        CreateSphere(gl,draw_model.sphere.vao,draw_model.sphere.vbo,draw_model.sphere.ebo,draw_model.sphere.index_count);
        LoadMaterial(draw_model.rusted_iron,"rusted_iron");
        LoadMaterial(draw_model.gold,"gold");
        LoadMaterial(draw_model.grass,"grass");
        LoadMaterial(draw_model.plastic,"plastic");
        LoadMaterial(draw_model.wall,"wall");
    }
    
    void init(){
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);//defualt is GL_LESS, this is for skybox trick
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);//无缝立方体贴图

        createEnvCubeMap();

        createDiffuseMap();
        createSpecularMap();
        ibl.pbr_shader = std::make_unique<Shader>(
           (ShaderPath+"ibl_pbr_v.glsl").c_str(),
           (ShaderPath+"ibl_pbr_f.glsl").c_str() 
        );

        createDrawModel();

        glViewport(0,0,gl->Width(),gl->Height());

        ibl.pbr_shader->use();
        ibl.pbr_shader->setInt("albedoMap",albedo_tex_unit);
        ibl.pbr_shader->setInt("normalMap",normal_tex_unit);
        ibl.pbr_shader->setInt("metallicMap",metallic_tex_unit);
        ibl.pbr_shader->setInt("roughnessMap",roughness_tex_unit);
        ibl.pbr_shader->setInt("aoMap",ao_tex_unit);
        ibl.pbr_shader->setInt("irradianceMap",irradiance_tex_unit);
        ibl.pbr_shader->setInt("prefilterMap",prefilter_tex_unit);
        ibl.pbr_shader->setInt("brdfLUT",lut_tex_unit);

        GL_CHECK
    }
    void setCamera(const std::unique_ptr<control::FPSCamera>& camera){
        auto view = camera->getViewMatrix();
        auto proj = glm::perspective(glm::radians(camera->getZoom()),(float)gl->Width()/gl->Height(),0.1f,100.f);
        cube_map.skybox_shader->use();
        cube_map.skybox_shader->setMat4("view",view);
        cube_map.skybox_shader->setMat4("projection",proj);
        ibl.pbr_shader->use();
        ibl.pbr_shader->setMat4("view",view);
        ibl.pbr_shader->setMat4("projection",proj);
        ibl.pbr_shader->setVec3("cameraPos",camera->getCameraPos());

    }
    void draw(){

        glBindFramebuffer(GL_FRAMEBUFFER,0);

        ibl.pbr_shader->use();

        glBindTextureUnit(irradiance_tex_unit,ibl.irradiance_map);
        glBindTextureUnit(prefilter_tex_unit,ibl.prefilter_map);
        glBindTextureUnit(lut_tex_unit,ibl.lut_map);

        //rusted iron
        glBindTextureUnit(albedo_tex_unit,draw_model.rusted_iron.albeodo);
        glBindTextureUnit(normal_tex_unit,draw_model.rusted_iron.normal);
        glBindTextureUnit(metallic_tex_unit,draw_model.rusted_iron.metallic);
        glBindTextureUnit(roughness_tex_unit,draw_model.rusted_iron.roughness);
        glBindTextureUnit(ao_tex_unit,draw_model.rusted_iron.ao);

        auto model = glm::translate(glm::mat4(1.f),glm::vec3(-5.f,0.f,2.f));
        ibl.pbr_shader->setMat4("model",model);
        GL_EXPR(glBindVertexArray(draw_model.sphere.vao));
        GL_EXPR(glDrawElements(GL_TRIANGLE_STRIP,draw_model.sphere.index_count,GL_UNSIGNED_INT,nullptr));

        //gold
        glBindTextureUnit(albedo_tex_unit,draw_model.gold.albeodo);
        glBindTextureUnit(normal_tex_unit,draw_model.gold.normal);
        glBindTextureUnit(metallic_tex_unit,draw_model.gold.metallic);
        glBindTextureUnit(roughness_tex_unit,draw_model.gold.roughness);
        glBindTextureUnit(ao_tex_unit,draw_model.gold.ao);

        model = glm::translate(glm::mat4(1.f),glm::vec3(-2.f,0.f,2.f));
        ibl.pbr_shader->setMat4("model",model);
        GL_EXPR(glBindVertexArray(draw_model.sphere.vao));
        GL_EXPR(glDrawElements(GL_TRIANGLE_STRIP,draw_model.sphere.index_count,GL_UNSIGNED_INT,nullptr));

        //grass
        glBindTextureUnit(albedo_tex_unit,draw_model.grass.albeodo);
        glBindTextureUnit(normal_tex_unit,draw_model.grass.normal);
        glBindTextureUnit(metallic_tex_unit,draw_model.grass.metallic);
        glBindTextureUnit(roughness_tex_unit,draw_model.grass.roughness);
        glBindTextureUnit(ao_tex_unit,draw_model.grass.ao);

        model = glm::translate(glm::mat4(1.f),glm::vec3(1.f,0.f,2.f));
        ibl.pbr_shader->setMat4("model",model);
        GL_EXPR(glBindVertexArray(draw_model.sphere.vao));
        GL_EXPR(glDrawElements(GL_TRIANGLE_STRIP,draw_model.sphere.index_count,GL_UNSIGNED_INT,nullptr));

        //plastic
        glBindTextureUnit(albedo_tex_unit,draw_model.plastic.albeodo);
        glBindTextureUnit(normal_tex_unit,draw_model.plastic.normal);
        glBindTextureUnit(metallic_tex_unit,draw_model.plastic.metallic);
        glBindTextureUnit(roughness_tex_unit,draw_model.plastic.roughness);
        glBindTextureUnit(ao_tex_unit,draw_model.plastic.ao);

        model = glm::translate(glm::mat4(1.f),glm::vec3(4.f,0.f,2.f));
        ibl.pbr_shader->setMat4("model",model);
        GL_EXPR(glBindVertexArray(draw_model.sphere.vao));
        GL_EXPR(glDrawElements(GL_TRIANGLE_STRIP,draw_model.sphere.index_count,GL_UNSIGNED_INT,nullptr));

        //wall
        glBindTextureUnit(albedo_tex_unit,draw_model.wall.albeodo);
        glBindTextureUnit(normal_tex_unit,draw_model.wall.normal);
        glBindTextureUnit(metallic_tex_unit,draw_model.wall.metallic);
        glBindTextureUnit(roughness_tex_unit,draw_model.wall.roughness);
        glBindTextureUnit(ao_tex_unit,draw_model.wall.ao);

        model = glm::translate(glm::mat4(1.f),glm::vec3(7.f,0.f,2.f));
        ibl.pbr_shader->setMat4("model",model);
        GL_EXPR(glBindVertexArray(draw_model.sphere.vao));
        GL_EXPR(glDrawElements(GL_TRIANGLE_STRIP,draw_model.sphere.index_count,GL_UNSIGNED_INT,nullptr));

        cube_map.skybox_shader->use();
        glBindVertexArray(cube_map.vao);
        glBindTextureUnit(0, cube_map.env_cube);
        glDrawArrays(GL_TRIANGLES,0,36);

        GL_CHECK
    }
};

class IBLApplication final:public Demo{
    void initResource() override{
        glEnable(GL_DEPTH_TEST);

        direct_renderer.init();
        ibl_renderer.init();

        camera = std::make_unique<control::FPSCamera>(glm::vec3{0.f,0.f,3.f});
    }
    void process_input() override;
    void render_frame() override;
    void render_imgui() override;

    enum class DrawType:int{
        None = 0,
        Direct = 1,
        IBL = 2,
        MIX = 3
    };
    DrawType draw_type{0};
    DirectRenderer direct_renderer{this->gl};
    IBLRenderer ibl_renderer{this->gl};
};

void IBLApplication::process_input(){

      static auto t = glfwGetTime();
      auto cur_t = glfwGetTime();
      auto delta_t = cur_t - t;
      t = cur_t;

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

void IBLApplication::render_frame(){
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glClearColor(0.f,0.f,0.f,0.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    switch (draw_type)
    {
        case DrawType::Direct:{
            direct_renderer.setCamera(camera);
            direct_renderer.draw();
        }
        break;
        case DrawType::IBL:{
            ibl_renderer.setCamera(camera);
            ibl_renderer.draw();
        }
        break;
        case DrawType::MIX:{

        }
        break;
        default:
            break;
    }

}

void IBLApplication::render_imgui(){
    ImGui::Text("IBL");
    ImGui::Text("fps: %f",ImGui::GetIO().Framerate);
    ImGui::Text("Draw Type");
    ImGui::RadioButton("Direct Light",reinterpret_cast<int*>(&draw_type),1);
    ImGui::RadioButton("IBL",reinterpret_cast<int*>(&draw_type),2);
}


int main(int argc,char** argv){
    try{
        IBLApplication app;
        app.run();
    }
    catch(const std::exception& err){
        LOG_ERROR("{}",err.what());
    }
    
    return 0;
}