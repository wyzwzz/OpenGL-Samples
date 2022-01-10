//
// Created by wyz on 2021/11/11.
//
#include "demo.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "logger.hpp"
void Demo::initGL()
{
    gl=GL::New();
    {
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGuiIO &io = ImGui::GetIO();

      ImGui::StyleColorsDark();
      ImGui_ImplGlfw_InitForOpenGL(gl->GetWindow(),true);
      ImGui_ImplOpenGL3_Init();
    }

    window_w = gl->Width();

    window_h = gl->Height();

    GL::MouseEvent=[&]( void *, MouseButton buttons, EventAction action, int xpos, int ypos){
      static bool left_button_press;
      if(buttons==Mouse_Left && action == Press){
          left_button_press=true;
          camera->processMouseButton(control::CameraDefinedMouseButton::Left,true,xpos,ypos);
      }
      else if(buttons==Mouse_Left && action==Release){
          left_button_press=false;
          camera->processMouseButton(control::CameraDefinedMouseButton::Left,false,xpos,ypos);
      }
      else if(action==Move && buttons==Mouse_Left && left_button_press){
          camera->processMouseMove(xpos,ypos);
      }
    };

    GL::ScrollEvent=[&](void*,int x,int y){
      camera->processMouseScroll(y);
    };

    GL::KeyboardEvent = [&]( void *, KeyButton key, EventAction action ){
      if(key==Key_Esc && action==Press){
          gl->Close();
      }
      if(action!=Press && action!=Repeat) return;
      switch (key)
      {
      case Key_W:camera->processKeyEvent(control::CameraDefinedKey::Forward,0.01);break;
      case Key_S:camera->processKeyEvent(control::CameraDefinedKey::Backward,0.01);break;
      case Key_A:camera->processKeyEvent(control::CameraDefinedKey::Left,0.01);break;
      case Key_D:camera->processKeyEvent(control::CameraDefinedKey::Right,0.01);break;
      case Key_Q:camera->processKeyEvent(control::CameraDefinedKey::Up,0.01);break;
      case Key_E:camera->processKeyEvent(control::CameraDefinedKey::Bottom,0.01);break;
      default:break;
      }

    };

    GL::FileDropEvent = [&]( void *, int count, const char **df ){

    };

    GL::FramebufferResizeEvent = []( void *, int width, int height ) {
      glBindFramebuffer(GL_FRAMEBUFFER,0);
      glViewport(0,0,width,height);
    };

}
Demo::Demo()
{
    initGL();
}
void Demo::begin_imgui(){
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Demo::end_imgui(){
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}