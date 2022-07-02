//
// Created by wyz on 2022/7/4.
//
#include "lut.hpp"

#include <demo.hpp>
#include <shader_program.hpp>
//#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <logger.hpp>
#include <imgui.h>
#include <array>
#include <fstream>
#include <random>
#include <vector>

class ColorGradingAPP: public Demo{
  public:
    ColorGradingAPP() = default;
    void initResource() override;
    void render_frame() override ;
    void render_imgui() override;

  private:
    void process_input() override;

    std::unique_ptr<Shader> post_process_shader;

};
void ColorGradingAPP::initResource()
{
    gl->SetVSync(0);

    GL_EXPR(glEnable(GL_DEPTH_TEST));

    post_process_shader = std::make_unique<Shader>("quad.vert","post_process.frag");
    post_process_shader->use();


}
void ColorGradingAPP::render_frame()
{

}
void ColorGradingAPP::render_imgui()
{

}
void ColorGradingAPP::process_input()
{
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

int main(){
    try
    {
        ColorGradingAPP app;
        app.run();
    }
    catch (const std::exception& err)
    {
        std::cerr<<err.what()<<std::endl;
    }
    return 0;
}