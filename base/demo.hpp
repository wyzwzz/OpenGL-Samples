//
// Created by wyz on 2021/11/11.
//
#pragma once
#include "GLImpl.hpp"
#include "mesh.hpp"
#include "camera.hpp"
using namespace gl;
using GL=GLContext<GLFWImpl,GLADImpl>;
class Demo{
  protected:
    void initGL();
    virtual void initResource() {}
    std::unique_ptr<control::FPSCamera> camera;
    decltype(GL::New()) gl;
    int window_w,window_h;
  public:
    Demo();
    void run(){
        initResource();
        try{
            while(!gl->Wait()){
                process_input();

                render_frame();
                
                begin_imgui();

                render_imgui();

                end_imgui();

                gl->Present();
                gl->DispatchEvent();
            }
        }
        catch (const std::exception& err)
        {
            std::cout<<err.what()<<std::endl;
        }
        catch (...)
        {

        }
    }
    virtual void process_input(){}
    virtual void render_frame(){}
    virtual void render_imgui(){}
    private:
    void begin_imgui();
    void end_imgui();
};