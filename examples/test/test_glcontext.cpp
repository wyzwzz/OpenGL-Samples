//
// Created by wyz on 2021/10/12.
//
#include <GLImpl.hpp>
#include <array>
#include <shader_program.hpp>
using namespace gl;
using GL=GLContext<GLFWImpl,GLADImpl>;
int main(){

    auto gl=GL::New();
    GL_CHECK
    GL::MouseEvent=[&]( void *, MouseButton buttons, EventAction action, int xpos, int ypos){

    };
    GL::KeyboardEvent = [&]( void *, KeyButton key, EventAction action ){
        bool show=true;
        if(key==Key_K && action==Press){
            show = !show;
            gl->Show(show);
        }

    };
    GL::FileDropEvent = [&]( void *, int count, const char **df ){

    };
    GL::FramebufferResizeEvent = []( void *, int width, int height ) {
      glBindFramebuffer(GL_FRAMEBUFFER,0);
      glViewport(0,0,width,height);
    };

    std::vector<std::array<float,3>> camera_points={
                                                    {1.f,0.5f,1.f},
                                                    {0.5f,0.f,0.5f},
                                                    {0.5f,1.f,1.f}};

    auto vao = gl->CreateVertexArray();

    glBindVertexArray(vao);
    auto vbo = gl->CreateBuffer();
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,9*sizeof(float),camera_points.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),0);
    glEnableVertexAttribArray(0);
    GL_CHECK
    Shader shader(
        "D:\\Projects\\OpenGLSamples\\examples\\test\\line_shader_v.glsl",
        "D:\\Projects\\OpenGLSamples\\examples\\test\\line_shader_f.glsl"
        );
    GL_CHECK
    while(!gl->Wait()){

        GL_EXPR(glClearColor(0.f,0.f,0.f,0.f));
        GL_EXPR(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));

        GL_EXPR(glBindVertexArray(vao));
        shader.use();
        GL_EXPR(glDrawArrays(GL_TRIANGLES,0,3));

        gl->Present();
        gl->DispatchEvent();
    }

    return 0;
}
