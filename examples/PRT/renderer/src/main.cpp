#include <demo.hpp>
#include <mesh.hpp>
#include <logger.hpp>

class PRTApplication final:public Demo{
    void initResource() override;
    void process_input() override;
    void render_frame() override;
    void render_imgui() override;

    struct Vertex{
        vec3f pos;
        vec3f normal;
        vec2f tex_coord;

    };
    struct{
        GL::GLVertexArray vao;
        GL::GLBuffer vbo;
        GL::GLBuffer ebo;

    }draw_model;


    void loadPRTResource(){

    }
    
};

void PRTApplication::initResource(){

}

void PRTApplication::process_input(){

}

void PRTApplication::render_frame(){

}

void PRTApplication::render_imgui(){

}

int main(){
    try{
        PRTApplication app;
        app.run();
    }
    catch(const std::exception& err){
        LOG_ERROR("{}",err.what());
    }
    return 0;
}
