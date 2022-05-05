//
// Created by wyz on 2022/5/4.
//
#include <demo.hpp>
#include <shader_program.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <logger.hpp>
#include <imgui.h>
#include <array>

static const float PI = 3.14159265359f;
struct Sphere{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uv;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;
};
static Sphere MakeSphere(){
    static constexpr uint32_t X_SEGMENTS = 64;
    static constexpr uint32_t Y_SEGMENTS = 64;
    static constexpr float PI = 3.14159265359f;

    Sphere sphere;

    for(uint32_t y = 0; y <= Y_SEGMENTS; y++){
        for(uint32_t x = 0; x <= X_SEGMENTS; x++){
            float x_segment = static_cast<float>(x) / X_SEGMENTS;
            float y_segment = static_cast<float>(y) / Y_SEGMENTS;
            float x_pos = std::cos(x_segment * 2.f * PI) * std::sin(y_segment * PI);
            float y_pos = std::cos(y_segment * PI);
            float z_pos = std::sin(x_segment * 2.f * PI) * std::sin(y_segment * PI);

            sphere.positions.emplace_back(x_pos,y_pos,z_pos);
            sphere.uv.emplace_back(x_segment,y_segment);
            sphere.normals.emplace_back(x_pos,y_pos,z_pos);
        }
    }
    //use GL_TRIANGLE_STRIP
    bool odd_row = false;
    for(uint32_t y = 0; y < Y_SEGMENTS; y++){
        if(!odd_row){
            for(uint32_t x = 0; x <= X_SEGMENTS; x++){
                sphere.indices.emplace_back( y    * (X_SEGMENTS + 1) + x);
                sphere.indices.emplace_back((y+1) * (X_SEGMENTS + 1) + x);
            }
        }
        else{
            for(int x = X_SEGMENTS; x>=0; x--){
                sphere.indices.emplace_back((y + 1) * (X_SEGMENTS + 1) + x);
                sphere.indices.emplace_back( y      * (X_SEGMENTS + 1) + x);
            }
        }
        odd_row = !odd_row;
    }

    return sphere;
}
static void CreateSphere(const std::shared_ptr<GL>& gl,GL::GLVertexArray& vao,GL::GLBuffer& vbo,GL::GLBuffer& ebo,int& index_count){
    auto sphere = MakeSphere();
    index_count = sphere.indices.size();
    vao = gl->CreateVertexArray();
    glBindVertexArray(vao);
    vbo = gl->CreateBuffer();
    ebo = gl->CreateBuffer();

    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    assert(sphere.positions.size()==sphere.normals.size() && sphere.normals.size() == sphere.uv.size());
    auto vertex_count = sphere.positions.size();
    glBufferData(GL_ARRAY_BUFFER,vertex_count*sizeof(float)*8,nullptr,GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER,0,vertex_count*3*sizeof(float),sphere.positions.data());
    glBufferSubData(GL_ARRAY_BUFFER,vertex_count*3*sizeof(float),vertex_count*3*sizeof(float),sphere.normals.data());
    glBufferSubData(GL_ARRAY_BUFFER,vertex_count*6*sizeof(float),vertex_count*2*sizeof(float),sphere.uv.data());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,index_count*sizeof(uint32_t),sphere.indices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)(vertex_count*3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)(vertex_count*6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

static void CreateTexture(const std::shared_ptr<GL>& gl,GL::GLTexture& tex,const std::string& path,GLuint filter,bool mipmap = false){
    stbi_set_flip_vertically_on_load(true);
    int width,height,nrComponents;
    auto data = stbi_load(path.c_str(),&width,&height,&nrComponents,0);
    if(data){
        tex = gl->CreateTexture(GL_TEXTURE_2D);
        GLenum format;
        if(nrComponents == 1) format = GL_RED;
        else if(nrComponents == 3) format = GL_RGB;
        else if(nrComponents == 4) format = GL_RGBA;
        else{
            LOG_ERROR("Invalid texture nrComponets: {}",nrComponents);
            format = 0;
        }
        glBindTexture(GL_TEXTURE_2D,tex);
        glTexImage2D(GL_TEXTURE_2D,0,format,width,height,0,format,GL_UNSIGNED_BYTE,data);
        if(mipmap)
            glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        //mipmap
        if(mipmap)
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        else
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter);

        stbi_image_free(data);
        LOG_INFO("Successfully create texture: {}",path.substr(1+path.find_last_of("/")));
    }
    else{
        LOG_ERROR("Load texture {} failed!",path);
    }
}

class BloomApplication final : public Demo{
  public:
    BloomApplication() = default;
    void initResource() override;
    void render_frame() override ;
    void render_imgui() override;
  private:
    void process_input() override;

    void createScreenQuad();

    void createFramebuffer();

    struct{
        GL::GLVertexArray vao;
        GL::GLBuffer vbo;
    }screen_quad;

    struct Framebuffer{
        GL::GLFramebuffer fbo;
        GL::GLRenderbuffer rbo;
        GL::GLTexture tex;
    };
    struct{
        std::unique_ptr<Shader> brightness_shader;
        std::unique_ptr<Shader> blur_shader;
        std::unique_ptr<Shader> bloom_shader;
        float brightness_threshold = 0.9f;
        GL::GLFramebuffer fbo;
        GL::GLRenderbuffer rbo;
        GL::GLTexture color;
        GL::GLTexture blur;
        int max_bloom_levels = 4;
        std::vector<Framebuffer> down_fb;
        float bloom_factor = 0.5f;
        float exposure = 1.0;
        int blur_pass = 10;
    }bloom;

    std::unique_ptr<Shader> composite_shader;

    struct{
        Sphere skybox;
        GL::GLVertexArray vao;
        GL::GLBuffer vbo;
        GL::GLBuffer ebo;
        GL::GLTexture env_map;
        int index_count;
        std::unique_ptr<Shader> shader;
    }sky;
    struct {
        Sphere light;
        GL::GLVertexArray vao;
        GL::GLBuffer vbo;
        GL::GLBuffer ebo;
        int index_count;
        std::unique_ptr<Shader> shader;
    }light;
    int draw_mode = 0;
};
void BloomApplication::createFramebuffer()
{
    if(bloom.down_fb.size() == bloom.max_bloom_levels) return;
    std::cerr<<"create new framebuffer"<<std::endl;
    bloom.down_fb.clear();
    auto w = gl->Width();
    auto h = gl->Height();
    for(int i = 0; i < bloom.max_bloom_levels; i++){
        bloom.down_fb.emplace_back();
        auto& fb = bloom.down_fb.back();
        fb.fbo = gl->CreateFramebuffer();
        fb.rbo = gl->CreateRenderbuffer();
        GL_EXPR(glBindFramebuffer(GL_FRAMEBUFFER,fb.fbo));
        GL_EXPR(glBindRenderbuffer(GL_RENDERBUFFER,fb.rbo));
        GL_EXPR(glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH24_STENCIL8,w,h));
        GL_EXPR(glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,bloom.rbo));
        fb.tex = gl->CreateTexture(GL_TEXTURE_2D);
        GL_EXPR(glTextureStorage2D(fb.tex,1,GL_RGBA32F,w,h));
        glTextureParameteri(fb.tex,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTextureParameteri(fb.tex,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTextureParameteri(fb.tex,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTextureParameteri(fb.tex,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        GL_EXPR(glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,fb.tex,0));
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){
            throw std::runtime_error("Framebuffer object is not complete!");
        }
        w /= 2;
        h /= 2;
    }

}
void BloomApplication::createScreenQuad()
{
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
void BloomApplication::process_input()
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
void BloomApplication::initResource()
{
    gl->SetWindowTitle("Bloom");
    GL_EXPR(glEnable(GL_DEPTH_TEST));
    glDepthFunc(GL_LEQUAL);//defualt is GL_LESS, this is for skybox trick

    camera = std::make_unique<control::FPSCamera>(glm::vec3{0.f,0.f,6.f});

    const std::string ShaderPath = "C:\\Users\\wyz\\projects\\OpenGL-Samples\\examples\\Bloom\\shader/";


    bloom.brightness_shader = std::make_unique<Shader>(
        (ShaderPath+"screenquad.vert").c_str(),
        (ShaderPath+"brightness.frag").c_str()
        );
    bloom.fbo = gl->CreateFramebuffer();
    bloom.rbo = gl->CreateRenderbuffer();
    GL_EXPR(glBindFramebuffer(GL_FRAMEBUFFER,bloom.fbo));
    GL_EXPR(glBindRenderbuffer(GL_RENDERBUFFER,bloom.rbo));
    GL_EXPR(glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH24_STENCIL8,gl->Width(),gl->Height()));
    GL_EXPR(glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,bloom.rbo));
    bloom.color = gl->CreateTexture(GL_TEXTURE_2D);
    auto w = gl->Width();
    auto h = gl->Height();
    GL_EXPR(glTextureStorage2D(bloom.color,1,GL_RGBA32F,gl->Width(),gl->Height()));
//    GL_EXPR(glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,bloom.color,0));
    bloom.blur = gl->CreateTexture(GL_TEXTURE_2D);
    GL_EXPR(glTextureStorage2D(bloom.blur,1,GL_RGBA32F,gl->Width(),gl->Height()));
    glTextureParameteri(bloom.blur,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(bloom.blur,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(bloom.blur,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTextureParameteri(bloom.blur,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){
        throw std::runtime_error("Framebuffer object is not complete!");
    }
    GL_CHECK

    std::string env_map_path = "C:\\Users\\wyz\\projects\\OpenGL-Samples\\data\\textures\\Barcelona_Rooftops\\Barce_Rooftop_C_8k.jpg";
    sky.skybox = MakeSphere();
    CreateSphere(gl,sky.vao,sky.vbo,sky.ebo,sky.index_count);
    CreateTexture(gl,sky.env_map,env_map_path,GL_LINEAR);
    sky.shader = std::make_unique<Shader>((ShaderPath+"background_v.glsl").c_str(),
                                          (ShaderPath+"equirectangular2cubemap_f.glsl").c_str());

    sky.shader->use();
    sky.shader->setInt("equirectangularMap",0);

    light.light = MakeSphere();
    CreateSphere(gl,light.vao,light.vbo,light.ebo,light.index_count);
    light.shader = std::make_unique<Shader>((ShaderPath+"light.vert").c_str(),
                                          (ShaderPath+"light.frag").c_str());


    bloom.blur_shader = std::make_unique<Shader>(
        (ShaderPath+"screenquad.vert").c_str(),
        (ShaderPath+"blur.frag").c_str()
        );
    bloom.blur_shader->use();
    bloom.blur_shader->setInt("blurTexture",0);


    bloom.bloom_shader = std::make_unique<Shader>(
        (ShaderPath+"screenquad.vert").c_str(),
        (ShaderPath+"bloom.frag").c_str()
        );
    bloom.bloom_shader->use();
    bloom.bloom_shader->setInt("blurTexture",0);
    bloom.bloom_shader->setInt("colorTexture",1);

    composite_shader = std::make_unique<Shader>(
        (ShaderPath+"screenquad.vert").c_str(),
        (ShaderPath+"composite.frag").c_str()
        );
    composite_shader->use();
    composite_shader->setInt("blurTexture",0);
    composite_shader->setInt("renderTexture",1);
    composite_shader->setVec2("resolution",gl->Width(),gl->Height());

    createScreenQuad();

    GL_CHECK
}
void BloomApplication::render_frame()
{
    createFramebuffer();

    glBindFramebuffer(GL_FRAMEBUFFER,0);
//    glBindRenderbuffer(GL_RENDERBUFFER,0);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
//    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glClearColor(0.f,0.f,0.f,0.f);
    glClearDepthf(1.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    auto view = camera->getViewMatrix();
    auto proj = glm::perspective(glm::radians(camera->getZoom()),(float)gl->Width()/gl->Height(),0.1f,100.f);
    float width = gl->Width();
    float height = gl->Height();

    glBindFramebuffer(GL_FRAMEBUFFER,bloom.fbo);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glViewport(0,0,width,height);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,bloom.color,0);

    light.shader->use();
    light.shader->setMat4("view",view);
    light.shader->setMat4("projection",proj);
    GL_EXPR(glBindVertexArray(light.vao));
    GL_EXPR(glDrawElements(GL_TRIANGLE_STRIP,light.index_count,GL_UNSIGNED_INT,nullptr));
    GL_CHECK

    sky.shader->use();
    sky.shader->setMat4("view",view);
    sky.shader->setMat4("projection",proj);
    glBindTextureUnit(0,sky.env_map);
    glBindVertexArray(sky.vao);
//    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glDrawElements(GL_TRIANGLE_STRIP,sky.index_count,GL_UNSIGNED_INT,nullptr);


    glDisable(GL_DEPTH_TEST);
    //draw brightness into bloom.blur
    glBindVertexArray(screen_quad.vao);
    bloom.brightness_shader->use();
    bloom.brightness_shader->setVec2("resolution",width,height);
    bloom.brightness_shader->setFloat("threshold",bloom.brightness_threshold);
    glBindTextureUnit(0,bloom.color);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,bloom.blur,0);
//    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glDrawArrays(GL_TRIANGLES,0,6);
    glFinish();
    glGenerateTextureMipmap(bloom.blur);

    for(int i = bloom.max_bloom_levels - 1; i > 0; i--){
        auto w = width / (1 << i);
        auto h = height / (1 << i);
        bloom.blur_shader->use();

//        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,bloom.blur,0);
        glBindFramebuffer(GL_FRAMEBUFFER,bloom.down_fb[i].fbo);

        glViewport(0,0,w,h);
        for(int j= 0; j < bloom.blur_pass;j++){
            if(j == 0){
                glBindTextureUnit(0,bloom.blur);
                bloom.blur_shader->setInt("curLod", i);
            }
            else{
                glBindTextureUnit(0,bloom.down_fb[i].tex);
                bloom.blur_shader->setInt("curLod", 0);
            }
            bloom.blur_shader->setVec2("resolution", w, h);

            bloom.blur_shader->setBool("horizontal", true);

            glDrawArrays(GL_TRIANGLES, 0, 6);
            glFinish();
            bloom.blur_shader->setBool("horizontal", false);

            glDrawArrays(GL_TRIANGLES, 0, 6);
            glFinish();
        }

        glBindFramebuffer(GL_FRAMEBUFFER,bloom.down_fb[i-1].fbo);
        w *= 2;
        h *= 2;
        glViewport(0,0,w,h);
        bloom.bloom_shader->use();
        bloom.bloom_shader->setVec2("resolution",w,h);
        bloom.bloom_shader->setFloat("bloomFactor",bloom.bloom_factor);
        bloom.bloom_shader->setInt("curLod",i-1);
        glBindTextureUnit(0,bloom.down_fb[i].tex);
        glBindTextureUnit(1,bloom.blur);
        glDrawArrays(GL_TRIANGLES,0,6);

    }
    GL_CHECK



    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glViewport(0,0,width,height);
    composite_shader->use();
    glBindTextureUnit(0,bloom.down_fb[0].tex);
    glBindTextureUnit(1,bloom.color);
    composite_shader->setInt("mode",draw_mode);
    composite_shader->setFloat("exposure",bloom.exposure);
    glDrawArrays(GL_TRIANGLES,0,6);

    glBindFramebuffer(GL_FRAMEBUFFER,bloom.down_fb[0].fbo);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER,0);

    GL_CHECK

}
void BloomApplication::render_imgui()
{
    ImGui::Text("Bloom");
    ImGui::Text("fps: %f",ImGui::GetIO().Framerate);
    ImGui::Text("Draw Type");
    ImGui::RadioButton("Origin Color",&draw_mode,0);
    ImGui::RadioButton("Only Bloom Color",&draw_mode,1);
    ImGui::RadioButton("Bloom",&draw_mode,2);
    ImGui::SliderFloat("Exposure",&bloom.exposure,0.0,2.0);
    ImGui::InputInt("Down sample level",&bloom.max_bloom_levels);
    ImGui::SliderFloat("BloomFactor",&bloom.bloom_factor,0.0,100.0);
    ImGui::InputInt("Blur pass count",&bloom.blur_pass);
}


int main(){
    try
    {
        BloomApplication app;
        app.run();
    }
    catch (const std::exception& err)
    {
        std::cout<<err.what()<<std::endl;
    }
    return 0;
}