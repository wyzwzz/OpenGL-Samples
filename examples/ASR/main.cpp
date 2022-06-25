#include <demo.hpp>
#include <shader_program.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <logger.hpp>
#include <imgui.h>
#include <array>

class ASRApplication: public Demo{
  public:
    ASRApplication() = default;
    void initResource() override;
    void render_frame() override ;
    void render_imgui() override;
  private:
    void process_input() override;

  private:

};
void ASRApplication::initResource()
{

}
void ASRApplication::render_frame()
{

}
void ASRApplication::render_imgui()
{

}
void ASRApplication::process_input()
{

}

int main(){
    try
    {
        ASRApplication app;
        app.run();
    }
    catch (const std::exception& err)
    {
        std::cerr<<err.what()<<std::endl;
    }
    return 0;
}