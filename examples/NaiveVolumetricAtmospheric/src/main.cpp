//
// Created by wyz on 2022/4/25.
//

#include <demo.hpp>
#include <shader_program.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <logger.hpp>
#include <imgui.h>
#include <array>
const std::string ShaderPath = "C:/Users/wyz/projects/OpenGL-Samples/examples/NaiveVolumetricAtmospheric/shader/";
//no other model exist but only render sky
class NVAApplication final: public Demo{
  public:
    NVAApplication() = default;
    void initResource() override;
    void render_frame() override ;
    void render_imgui() override;

  private:
    void process_input() override;

    void createScreenQuad();

    struct{
        GL::GLVertexArray vao;
        GL::GLBuffer vbo;
    }screen_quad;
    struct{
        //glm::vec3 light_direction;//toward light
        glm::vec3 light_radiance = {2.f,2.f,2.f};
        float yaw = 270.f;//xz
        float pitch = 45.f;//y
    }sun;
    std::unique_ptr<Shader> nva_shader;

};
void NVAApplication::process_input()
{
    static auto t = glfwGetTime();
    auto cur_t = glfwGetTime();
    auto delta_t = cur_t - t;
    t = cur_t;
    delta_t *= 0.001;
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
void NVAApplication::initResource()
{
    gl->SetWindowTitle("Naive Volumetric Atmosphere Scattering");
    GL_EXPR(glEnable(GL_DEPTH_TEST));

    nva_shader = std::make_unique<Shader>(
        (ShaderPath+"atmospheric_scattering_test.vert").c_str(),
        (ShaderPath+"atmospheric_scattering_test.frag").c_str()
        );
    nva_shader->use();
    nva_shader->setVec2("WindowSize",window_w,window_h);

    createScreenQuad();

    camera = std::make_unique<control::FPSCamera>(glm::vec3{0.f, 6.53f, 0.f});
}
void NVAApplication::render_frame()
{
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glClearColor(1.f,0.f,0.f,0.f);
    glClearDepthf(1.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    nva_shader->use();
    nva_shader->setVec3("CameraPos",camera->getCameraPos());
    nva_shader->setVec3("CameraRight",camera->getCameraRight());
    nva_shader->setVec3("CameraUp",camera->getCameraUp());
    nva_shader->setFloat("CameraFov",glm::radians(camera->getZoom()));
    nva_shader->setVec3("ViewDirection",glm::normalize(camera->getCameraLookAt()-camera->getCameraPos()));
    float y = std::sin(glm::radians(sun.pitch));
    float x = std::cos(glm::radians(sun.pitch)) * std::cos(glm::radians(sun.yaw));
    float z = std::cos(glm::radians(sun.pitch)) * std::sin(glm::radians(sun.yaw));
    nva_shader->setVec3("LightDirection",glm::normalize(glm::vec3(x,y,z)));
    nva_shader->setVec3("LightRadiance",sun.light_radiance);


    glBindVertexArray(screen_quad.vao);
    GL_EXPR(glDrawArrays(GL_TRIANGLES,0,6));

}
void NVAApplication::render_imgui()
{
    ImGui::Text("Naive Volumetric Atmosphere Renderer");
    ImGui::Text("FPS %.2f",ImGui::GetIO().Framerate);
    ImGui::SliderFloat("Light Pitch",&sun.pitch,0.f,89.f);
    ImGui::SliderFloat("Light Yaw",&sun.yaw,0.f,360.f);
    ImGui::SliderFloat3("Light Radiance",&sun.light_radiance.x,0.f,50.f);
    ImGui::Text("Camera Position %.2f %.2f %.2f",camera->getCameraPos().x,camera->getCameraPos().y,camera->getCameraPos().z);

}
void NVAApplication::createScreenQuad()
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
    screen_quad.vao = gl->CreateVertexArray();
    GL_CHECK
    glBindVertexArray(screen_quad.vao);
    GL_CHECK
    screen_quad.vbo = gl->CreateBuffer();
    glBindBuffer(GL_ARRAY_BUFFER,screen_quad.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screen_quad_vertices), screen_quad_vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    GL_CHECK
}

int main(){
    try
    {
        NVAApplication app;
        app.run();
    }
    catch (const std::exception& err)
    {
        std::cout<<err.what()<<std::endl;
    }
    return 0;
}