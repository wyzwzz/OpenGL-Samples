#pragma once
#include "GLImpl.hpp"
#include "camera.hpp"
#include "mesh.hpp"
#include "benchmark.hpp"
using namespace gl;
using GL = GLContext<GLFWImpl, GLADImpl>;
class Demo
{
  protected:
    void initGL();
    virtual void initResource()
    {
    }

    decltype(GL::New()) gl;
    int window_w, window_h;
    std::unique_ptr<control::FPSCamera> camera;
    Benchmark bench_mark;
  public:
    Demo();
    
    void run()
    {
        initResource();
        try
        {
            while (!gl->Wait())
            {
                process_input();

                _render_frame();

                begin_imgui();

                render_imgui();

                end_imgui();

                gl->Present();
                gl->DispatchEvent();
            }
        }
        catch (const std::exception &err)
        {
            LOG_ERROR("{}", err.what());
        }
        catch (...)
        {
        }
    }
  protected:
    virtual void process_input()
    {
    }
    virtual void render_frame()
    {
    }
    virtual void render_imgui()
    {
    }

  private:
    void _render_frame();
    void begin_imgui();
    void end_imgui();
};