#pragma once
#include <functional>
#include <chrono>
#include "logger.hpp"
namespace gl
{

class Benchmark
{
  public:
    bool isActive() const{ return active; }

    void start(){
        active = true;
        LOG_INFO("=== Start benchmark ===");
    }

    void stop(){
        LOG_INFO("=== Benchmark finished ===");
        LOG_INFO("\truntime: {}s",run_times/1000.0);
        LOG_INFO("\tframes : {}",frame_count);
        LOG_INFO("\tfps    : {}",frame_count/(run_times/1000.0));
        LOG_INFO("\tframe t: {}ms",run_times/frame_count);
        LOG_INFO("==========================");
        active = false;
        frame_count = 0;
        run_times = 0.0;
    }

    void run(std::function<void()> render_func){
        auto start_t = std::chrono::high_resolution_clock::now();
        render_func();
        auto stop_t = std::chrono::high_resolution_clock::now();
        auto delta_t = std::chrono::duration<double,std::milli>(stop_t - start_t).count();
        frame_count++;
        run_times += delta_t;
    }

  private:
    bool active{false};
    int frame_count{0};
    double run_times{0.0};
};

} // namespace gl
