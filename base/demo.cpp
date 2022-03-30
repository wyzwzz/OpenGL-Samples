#include "demo.hpp"
#include "logger.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
void Demo::initGL()
{
    // init window and gl context
    gl = GL::New();

    // init imgui
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();

        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(gl->GetWindow(), true);
        ImGui_ImplOpenGL3_Init();
    }

    window_w = gl->Width();

    window_h = gl->Height();

    // init default input event handle function
    GL::MouseEvent = [&](void *, MouseButton buttons, EventAction action, int xpos, int ypos) {
        static bool left_button_press;
        if (buttons == Mouse_Left && action == Press)
        {
            left_button_press = true;
            camera->processMouseButton(control::CameraDefinedMouseButton::Left, true, xpos, ypos);
        }
        else if (buttons == Mouse_Left && action == Release)
        {
            left_button_press = false;
            camera->processMouseButton(control::CameraDefinedMouseButton::Left, false, xpos, ypos);
        }
        else if (action == Move && buttons == Mouse_Left && left_button_press)
        {
            camera->processMouseMove(xpos, ypos);
        }
    };

    GL::ScrollEvent = [&](void *, int x, int y) { camera->processMouseScroll(y); };

    GL::KeyboardEvent = [&](void *, KeyButton key, EventAction action) {
        if (key == Key_Esc && action == Press)
        {
            gl->Close();
        }
        else if(key == Key_B && action == Press){
            if(bench_mark.isActive()){
                bench_mark.stop();
            }
            else{
                bench_mark.start();
            }
        }
    };

    GL::FileDropEvent = [&](void *, int count, const char **df) {

    };

    GL::FramebufferResizeEvent = [](void *, int width, int height) {
        GL_EXPR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        GL_EXPR(glViewport(0, 0, width, height));
    };
}
Demo::Demo()
{
    initGL();


}
void Demo::begin_imgui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Demo::end_imgui()
{
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Demo::_render_frame(){
    if(bench_mark.isActive()){
        bench_mark.run([this](){
            render_frame();
            });
    }
    else{
        render_frame();
    }
}