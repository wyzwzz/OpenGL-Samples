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

struct Sphere{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uv;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;
};

class DirectRenderer{
public:
    struct{
        GL::GLTexture albedo;
        GL::GLTexture normal;
        GL::GLTexture metallic;
        GL::GLTexture roughness;
        GL::GLTexture ao;
        GL::GLSampler sampler;
    }material;
    
    struct{
        glm::vec3 position;
        glm::vec3 color;
    }light;
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

    }

    void draw(){

    }
};

class IBLRenderer{
public:
    void init(){

    }
    void draw(){

    }
};

class IBLApplication final:public Demo{
    void initResource() override{
        glEnable(GL_DEPTH_TEST);

        direct_renderer.init();
        ibl_renderer.init();

        camera = std::make_unique<control::FPSCamera>(glm::vec3{0.f,0.f,1.f});
    }
    void render_frame() override;
    void render_imgui() override;


    DirectRenderer direct_renderer{};
    IBLRenderer ibl_renderer{};
};

void IBLApplication::render_frame(){
    glClearColor(0.f,0.f,0.f,0.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

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