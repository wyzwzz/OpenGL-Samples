//
// Created by wyz on 2021/11/2.
//
#define TINYOBJLOADER_IMPLEMENTATION
#include "shader_program.hpp"
#include "camera.hpp"
#include <tiny_obj_loader.h>
#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>
#include <GLImpl.hpp>
using namespace gl;
using GL=GLContext<GLFWImpl,GLADImpl>;
int main(){
    spdlog::stopwatch sw;
    std::string m_obj_path="C:\\Users\\wyz\\projects\\OpenGL-Samples\\data/testObj/testObj.obj";
    tinyobj::ObjReader obj_reader;
    if(!obj_reader.ParseFromFile(m_obj_path)){
        if(!obj_reader.Error().empty()){
            spdlog::error("TinyObjReader error: {0}",obj_reader.Error());
        }
        spdlog::error("Read obj file failed");
        exit(-1);
    }
    if(!obj_reader.Warning().empty()){
        spdlog::warn("TinyObjReader warning: {0}",obj_reader.Warning());
    }
    spdlog::info("Stopwatch: {} seconds",sw);
    sw.reset();

    auto& attrib = obj_reader.GetAttrib();
    auto& shapes = obj_reader.GetShapes();
    spdlog::info("mesh number: {0}",shapes.size());
    for(auto& it:shapes){
        spdlog::info("mesh {0} has triangle size: {1}",it.name,it.mesh.num_face_vertices.size());
    }
    spdlog::info("total vertex number: {0}",attrib.vertices.size()/3);
    std::vector<int> mesh_index;
    mesh_index.reserve(shapes[0].mesh.indices.size());
    for(auto& shape:shapes){
        auto& index=shape.mesh.indices;
        for(auto& idx:index){
            if(idx.normal_index!=idx.vertex_index){
                spdlog::critical("normal index not equal to vertex index!!!");
            }
            mesh_index.emplace_back(idx.vertex_index);
        }
    }
    spdlog::info("total mesh index number: {0}",mesh_index.size());
    size_t index_num = mesh_index.size();
    assert(attrib.vertices.size()==attrib.normals.size());

//----------------------------------------------------------------------------------------
    auto gl=GL::New();

    auto vao = gl->CreateVertexArray();
    GL_EXPR(glBindVertexArray(vao));
    auto vbo = gl->CreateBuffer();
    GL_EXPR(glBindBuffer(GL_ARRAY_BUFFER,vbo));
    GL_EXPR(glBufferData(GL_ARRAY_BUFFER,2*attrib.vertices.size()*sizeof(tinyobj::real_t),
                 nullptr,GL_STATIC_DRAW));
    GL_EXPR(glBufferSubData(GL_ARRAY_BUFFER,0,attrib.vertices.size()*sizeof(tinyobj::real_t),
                    attrib.vertices.data()));
    GL_EXPR(glBufferSubData(GL_ARRAY_BUFFER,attrib.vertices.size()*sizeof(tinyobj::real_t),
                    attrib.normals.size()*sizeof(tinyobj::real_t),
                    attrib.normals.data()));
    auto ebo = gl->CreateBuffer();
    GL_EXPR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo));
    GL_EXPR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,shapes[0].mesh.indices.size()*sizeof(int),
                 mesh_index.data(),GL_STATIC_DRAW));
    GL_EXPR(glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(tinyobj::real_t),0));
    GL_EXPR(glEnableVertexAttribArray(0));
    GL_EXPR(glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,3*sizeof(tinyobj::real_t),
                          (void*)(shapes[0].mesh.indices.size()*sizeof(int))));
    GL_EXPR(glEnableVertexAttribArray(1));
    GL_EXPR(glBindVertexArray(0));

    Shader mesh_shading("C:\\Users\\wyz\\projects\\OpenGL-Samples\\examples\\tinyobjloader\\shader\\mesh_shading_v.glsl",
                        "C:\\Users\\wyz\\projects\\OpenGL-Samples\\examples\\tinyobjloader\\shader\\mesh_shading_f.glsl");

    control::FPSCamera camera({0.f,0.f,1.f});

    GL::MouseEvent=[&]( void *, MouseButton buttons, EventAction action, int xpos, int ypos){
        static bool left_button_press;
        if(buttons==Mouse_Left && action == Press){
            left_button_press=true;
            camera.processMouseButton(control::CameraDefinedMouseButton::Left,true,xpos,ypos);
        }
        else if(buttons==Mouse_Left && action==Release){
            left_button_press=false;
            camera.processMouseButton(control::CameraDefinedMouseButton::Left,false,xpos,ypos);
        }
        else if(action==Move && buttons==Mouse_Left && left_button_press){
//            spdlog::info("mouse move {0} {1}",xpos,ypos);
            camera.processMouseMove(xpos,ypos);
        }
    };
    GL::ScrollEvent=[&](void*,int x,int y){
//        spdlog::info("scroll y: {0}",y);
        camera.processMouseScroll(y);
    };
    GL::KeyboardEvent = [&]( void *, KeyButton key, EventAction action ){
        if(key==Key_Esc && action==Press){
            gl->Close();
        }
        if(action!=Press && action!=Repeat) return;
        switch (key)
        {
        case Key_W:camera.processKeyEvent(control::CameraDefinedKey::Forward,0.01);break;
        case Key_S:camera.processKeyEvent(control::CameraDefinedKey::Backward,0.01);break;
        case Key_A:camera.processKeyEvent(control::CameraDefinedKey::Left,0.01);break;
        case Key_D:camera.processKeyEvent(control::CameraDefinedKey::Right,0.01);break;
        case Key_Q:camera.processKeyEvent(control::CameraDefinedKey::Up,0.01);break;
        case Key_E:camera.processKeyEvent(control::CameraDefinedKey::Bottom,0.01);break;
        default:break;
        }

    };
    GL::FileDropEvent = [&]( void *, int count, const char **df ){

    };
    GL::FramebufferResizeEvent = []( void *, int width, int height ) {
      glBindFramebuffer(GL_FRAMEBUFFER,0);
      glViewport(0,0,width,height);
    };


    while(!gl->Wait()){

        GL_EXPR(glClearColor(0.f,0.f,0.f,0.f));
        GL_EXPR(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));

        auto view = camera.getViewMatrix();
        auto projection = glm::perspective(glm::radians(camera.getZoom()),
                                           (float)gl->Width()/gl->Height(),
                                           1.f,30000.f);
        glm::mat4 mvp =projection*view;


        mesh_shading.use();
        mesh_shading.setMat4("MVPMatrix",mvp);
        GL_EXPR(glBindVertexArray(vao));

        GL_EXPR(glDrawElements(GL_TRIANGLES,index_num,GL_UNSIGNED_INT,0));

        gl->Present();
        gl->DispatchEvent();
    }

    return 0;
}