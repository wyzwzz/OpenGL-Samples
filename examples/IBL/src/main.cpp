//
// Created by wyz on 2021/12/20.
//
#include <iostream>
#include <vector>
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

        CreateTexture(gl,material.albedo,TexturePath+"rested_iron/albedo.png");
        CreateTexture(gl,material.normal,TexturePath+"rested_iron/normal.png");
        CreateTexture(gl,material.metallic,TexturePath+"rested_iron/metallic.png");
        CreateTexture(gl,material.roughness,TexturePath+"rested_iron/roughness.png");
        CreateTexture(gl,material.ao,TexturePath+"rested_iron/ao.png");
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
    struct{
        GL::GLFramebuffer fbo;
        GL::GLRenderbuffer rbo;
        GL::GLTexture hdr;
        GL::GLTexture env_cube;
        static constexpr uint32_t CUBE_SIZE = 512;
        std::unique_ptr<Shader> equirectangular2cubemap;
        GL::GLVertexArray vao;
        GL::GLBuffer vbo;
    }cube_map;
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
        static glm::mat4 captureProj = glm::perspective(glm::radians(90.f),1.f,0.1f,10.f);
        static glm::mat4 captureView[] = {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };
        cube_map.equirectangular2cubemap->use();
    }
    void createEnvCubeMap(){
        CreateCube(gl,cube_map.vao,cube_map.vbo);
        cube_map.fbo = gl->CreateFramebuffer();
        cube_map.rbo = gl->CreateRenderbuffer();
        glBindFramebuffer(GL_FRAMEBUFFER,cube_map.fbo);
        glBindRenderbuffer(GL_RENDERBUFFER,cube_map.rbo);
        glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,cube_map.CUBE_SIZE,cube_map.CUBE_SIZE);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,cube_map.rbo);
        GL_CHECK

        createHDRTexture(TexturePath+"hdr/newport_loft.hdr");

        cube_map.env_cube = gl->CreateTexture(GL_TEXTURE_CUBE_MAP);
        for(uint32_t i = 0;i < 6; i++)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,0,GL_RGB16F,
            cube_map.CUBE_SIZE,cube_map.CUBE_SIZE,0,GL_RGB,GL_FLOAT,nullptr);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        cube_map.equirectangular2cubemap = std::make_unique<Shader>(
            (ShaderPath+"cube_map_v.glsl").c_str(),
            (ShaderPath+"equirectangular2cubemap_f.glsl").c_str()
        );

        captureEnvCubeMap();
    }

    void init(){
        // createEnvCubeMap();
    }
    void setCamera(const std::unique_ptr<control::FPSCamera>& camera){

    }
    void draw(){

    }
};

class IBLApplication final:public Demo{
    void initResource() override{
        glEnable(GL_DEPTH_TEST);

        direct_renderer.init();
        ibl_renderer.init();

        camera = std::make_unique<control::FPSCamera>(glm::vec3{0.f,0.f,3.f});
    }
    void render_frame() override;
    void render_imgui() override;


    DirectRenderer direct_renderer{this->gl};
    IBLRenderer ibl_renderer{this->gl};
};

void IBLApplication::render_frame(){
    glClearColor(0.f,0.f,0.f,0.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);


    direct_renderer.setCamera(camera);
    direct_renderer.draw();

}

void IBLApplication::render_imgui(){
    ImGui::Text("IBL");
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