//
// Created by wyz on 2022/6/10.
//
#include <demo.hpp>
#include <shader_program.hpp>
#include <logger.hpp>
#include <imgui.h>
#include <array>
#include <texture.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tiny_gltf.h"


template<typename T>
using Box = std::unique_ptr<T>;

template<typename T, typename...Args>
Box<T> newBox(Args&&...args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}
template<typename T>
using RC = std::shared_ptr<T>;

template<typename T, typename...Args>
RC<T> newRC(Args&&...args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

class DrawModel{
  public:

  private:
    uint32_t vao = 0;
    uint32_t vbo = 0;
    uint32_t ebo = 0;
    uint32_t tex = 0;
};
class GLTexture2D{
  public:
    GLTexture2D(int w,int h,GLenum format,uint8_t* data,size_t size){
        glGenTextures(1,&id);
        glBindTexture(GL_TEXTURE_2D,id);
        glTextureStorage2D(id,1,format,w,h);
        glTextureSubImage2D(id,0,0,0,w,h,format,GL_UNSIGNED_BYTE,data);
        glBindTexture(GL_TEXTURE_2D,0);
    }
    int width() const {return w;}
    int height() const {return h;}
    int texID() const {return id;}
    void bind() const {
        assert(id);
        GL_EXPR(glBindTexture(GL_TEXTURE_2D,id));
    }
    void unbind() const{
        assert(id);
        GL_EXPR(glBindTexture(GL_TEXTURE_2D,0));
    }
  private:
    int w = 0;
    int h = 0;
    uint32_t id = 0;
};
struct Model{

};
GLenum get_gl_image_format(int comp,int type){
    assert(comp > 0);
    switch (type)
    {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:{
        switch (comp)
        {
        case 1:return GL_RED;
        case 2:return GL_RG;
        case 3:return GL_RGB;
        case 4:return GL_RGBA;
        default:assert(false);
        }
    }
    case TINYGLTF_COMPONENT_TYPE_FLOAT:{

    }
    default:
        assert(false);
    }
}
std::vector<RC<DrawModel>> load_from_gltf(const std::string& path){
    auto ext_idx = path.find_last_of('.');
    if (ext_idx == std::string::npos) {
        throw std::runtime_error("should have extension");
    }
    auto ext = path.substr(ext_idx);
    if (ext != ".glb" && ext != ".gltf") {
        throw std::runtime_error("Only support GLTF/GLB format.");
    }

    tinygltf::TinyGLTF loader;
    tinygltf::Model gltf_model;
    std::string err, warn;
    if (ext == ".glb") {
        loader.LoadBinaryFromFile(&gltf_model, &err, &warn, path);
    } else {
        loader.LoadASCIIFromFile(&gltf_model, &err, &warn, path);
    }
    if (!err.empty()) {
        throw std::runtime_error("Failed to load model: " + err);
    }
    if (!warn.empty()) {
        LOG_ERROR("Loading model warning: ");
    }

    for(auto& gltf_buffer_view:gltf_model.bufferViews){

    }
    std::vector<RC<GLTexture2D>> textures;
    std::unordered_map<std::string,RC<GLTexture2D>> texture_map;
    for(auto& gltf_texture:gltf_model.textures){
        auto& image = gltf_model.images[gltf_texture.source];
        auto& sampler = gltf_model.samplers[gltf_texture.sampler];
        if(texture_map.count(image.name) == 1){
            //pass
        }
        else{
            textures.emplace_back(newRC<GLTexture2D>(
                image.width,image.height, get_gl_image_format(image.component,image.pixel_type),
                image.image.data(),image.image.size()
                ));
            texture_map.insert({image.name,textures.back()});
        }
    }


}
class ClusteredShadingApp final : public Demo{
  public:
    ClusteredShadingApp() = default;
    void initResource() override;
    void render_frame() override ;
    void render_imgui() override;
  private:
    void process_input() override;


    Box<Shader> cluster_view_frustum_shader;
    Box<Shader> cluster_cull_light_shader;
    Box<Shader> deferred_gbuffer_shader;
    Box<Shader> deffered_render_shader;
};
void ClusteredShadingApp::initResource()
{
    std::string path = "C:\\Users\\wyz\\projects\\HybridRenderingEngine\\assets\\models\\Sponza\\Sponza.gltf";
    auto draw_models = load_from_gltf(path);
}
void ClusteredShadingApp::render_frame()
{
    Demo::render_frame();
}
void ClusteredShadingApp::render_imgui()
{
    Demo::render_imgui();
}
void ClusteredShadingApp::process_input()
{
    Demo::process_input();
}

int main(){
    try
    {
        ClusteredShadingApp app;
        app.run();
    }
    catch (const std::exception& err)
    {
        std::cout<<err.what()<<std::endl;
    }
    return 0;
}