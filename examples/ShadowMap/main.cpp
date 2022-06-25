//
// Created by wyz on 2021/11/11.
//
#include <demo.hpp>
#include <shader_program.hpp>
//#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <logger.hpp>
#include <imgui.h>
class ShadowMapApplication final: public Demo{
    void loadTexture(const std::string& path,GL::GLTexture& tex){
        int width,height,nrComponents;
        stbi_set_flip_vertically_on_load(true);
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
        // glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        //mipmap
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

        stbi_image_free(data);
        LOG_INFO("Successfully create texture: {}",path.substr(1+path.find_last_of("/")));
    }
    else{
        LOG_ERROR("Load texture {} failed!",path);
    }
    }
    void createVertexBuffer(const Mesh& mesh,GL::GLVertexArray& vao,GL::GLBuffer& vbo,GL::GLBuffer& ebo){

        vao = gl->CreateVertexArray();
        glBindVertexArray(vao);

        vbo = gl->CreateBuffer();
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,mesh.vertices.size()*sizeof(vertex_t),mesh.vertices.data(),GL_STATIC_DRAW);

        ebo = gl->CreateBuffer();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,mesh.indices.size()*sizeof(uint32_t),mesh.indices.data(),GL_STATIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),
                                (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),
                                (void*)(6*sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
        GL_CHECK
    }
    void initResource() override{
        gl->SetWindowTitle("ShadowMap");
        glEnable(GL_DEPTH_TEST);

        auto view = glm::lookAt(vec3f(0,0,2),vec3f(0,0,0),vec3f(0,1,0));

        auto marry = load_mesh_from_obj("C:\\Users\\wyz\\projects\\OpenGL-Samples\\data\\mary\\Marry.obj");

        for(int i = 0;i<9;i++)
        {
            models.emplace_back();
            auto &marry_model = models.back();
            marry_model.draw_count = marry.indices.size();
            auto t_marry = marry;
            int r = i / 3 - 1;
            int c = i % 3 - 1;
            auto model_matrix = glm::translate(glm::mat4(1.f),glm::vec3(r*3,0,c*3));
            for(auto& v:t_marry.vertices){
                v.pos = model_matrix * glm::vec4(v.pos,1.f);
            }
            createVertexBuffer(t_marry, marry_model.vao, marry_model.vbo, marry_model.ebo);
            loadTexture("C:/Users/wyz/projects/OpenGL-Samples/data/mary/MC003_Kozakura_Mari.png", marry_model.tex);
        }

        auto floor = load_mesh_from_obj("C:/Users/wyz/projects/OpenGL-Samples/data/floor/floor.obj");
        models.emplace_back();
        auto& floor_model = models.back();
        floor_model.draw_count = floor.indices.size();
        createVertexBuffer(floor,floor_model.vao,floor_model.vbo,floor_model.ebo);

        //init shadow fbo
        {
            shadow.fbo = gl->CreateFramebuffer();
            glBindFramebuffer(GL_FRAMEBUFFER,shadow.fbo);
            shadow.rbo = gl->CreateRenderbuffer();
            glBindRenderbuffer(GL_RENDERBUFFER,shadow.rbo);
            glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH24_STENCIL8,shadow_w,shadow_h);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,shadow.rbo);
            GL_CHECK
            shadow.tex = gl->CreateTexture(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D,shadow.tex);
            //texture binding 0
            glBindTextureUnit(0,shadow.tex);
            GL_CHECK
            glTextureStorage2D(shadow.tex,1,GL_RGBA32F,shadow_w,shadow_h);
            GL_CHECK
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            GL_CHECK
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,shadow.tex,0);
            GL_CHECK
            if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){
                std::cout<<std::hex<<glCheckFramebufferStatus(GL_FRAMEBUFFER)<<std::endl;
                throw std::runtime_error("Framebuffer object is not complete!");
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            GL_CHECK

        }
        shadow_shader = std::make_unique<Shader>(
            "C:\\Users\\wyz\\projects\\OpenGL-Samples\\examples\\shadowmap\\shader\\shadow_v.glsl",
            "C:\\Users\\wyz\\projects\\OpenGL-Samples\\examples\\shadowmap\\shader\\shadow_f.glsl"
            );
        phong_shader = std::make_unique<Shader>(
            "C:\\Users\\wyz\\projects\\OpenGL-Samples\\examples\\shadowmap\\shader\\phong_v.glsl",
            "C:\\Users\\wyz\\projects\\OpenGL-Samples\\examples\\shadowmap\\shader\\phong_f.glsl"
            );

        // light.position = glm::vec3(5.f,5.f,0.f);
        // light.direction = glm::normalize(glm::vec3{-2.f,-2.f,0.f});

        camera = std::make_unique<control::FPSCamera>(glm::vec3{0.f, 1.5f, 5.f});
//        camera = std::make_unique<control::FPSCamera>(light.position);
    }
    void render_frame() override ;
    void render_imgui() override;
  public:
    ShadowMapApplication() = default;

  private:

    void process_input(){

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
    struct Light{
        vec3f position;
        vec3f direction;
        vec3f incidence;//color rgb
    } light;
    float light_pitch = 45.f;

    std::unique_ptr<Shader> shadow_shader;
    std::unique_ptr<Shader> phong_shader;

    // std::vector<uint32_t> mesh_vao,mesh_vbo,mesh_ebo;

    struct Model{
        GL::GLVertexArray vao;
        GL::GLBuffer vbo;
        GL::GLBuffer ebo;
        GL::GLTexture tex;
        uint32_t draw_count = 0;
    };
    std::vector<Model> models;

    // uint32_t shadow_fbo,shadow_rbo,shadow_tex;

    struct{
        GL::GLFramebuffer fbo;
        GL::GLRenderbuffer rbo;
        GL::GLTexture tex;
        int method = 0;//0 for general, 1 for PCF, 2 for
    }shadow;

    const int shadow_w=4096,shadow_h=4096;
};
void ShadowMapApplication::render_imgui(){
    ImGui::Text("ShadowMap");
    ImGui::Text("FPS %.2f",ImGui::GetIO().Framerate);
    ImGui::SliderFloat("light pitch",&light_pitch,20.f,90.f,"%.0f");
    ImGui::RadioButton("General",&shadow.method,0);
    ImGui::RadioButton("PCF",&shadow.method,1);
    ImGui::RadioButton("PCSS",&shadow.method,2);
}
void ShadowMapApplication::render_frame()
{
    process_input();

    glClearColor(0.f,0.f,0.f,0.f);

    glBindFramebuffer(GL_FRAMEBUFFER,shadow.fbo);
    glViewport(0,0,shadow_w,shadow_h);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    float pitch = light_pitch / 180.f * 3.1415926f;
    float yaw = glfwGetTime();
    float y = std::sin(pitch);
    float x = std::cos(pitch) * std::cos(yaw);
    float z = std::cos(pitch) * std::sin(yaw);
    auto light_view = glm::lookAt(glm::vec3{x,y,z} * 5.f,{0.f,0.f,0.f},glm::normalize(glm::vec3{-1.f,1.f,0.f}));
//    auto light_projection = glm::perspective(glm::radians(30.f),float(shadow_w)/shadow_h,0.1f,50.f);
    auto light_projection = glm::ortho(-15.f,15.f,-15.f,15.f,0.1f,20.f);
    auto light_mvp = light_projection*light_view;
    shadow_shader->use();
    shadow_shader->setMat4("MVPMatrix",light_mvp);
    for(int i=0;i<models.size();i++){
        GL_EXPR(glBindVertexArray(models[i].vao));
        GL_EXPR(glDrawElements(GL_TRIANGLES,models[i].draw_count,GL_UNSIGNED_INT,0));
        GL_CHECK
    }
    GL_CHECK

    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glViewport(0,0,window_w,window_h);
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    auto camera_view = camera->getViewMatrix();
    auto camera_projection = glm::perspective(glm::radians(camera->getZoom()),float(window_w)/window_h,0.1f,100.f);
    auto camera_mvp = camera_projection * camera_view;
    phong_shader->use();
    phong_shader->setMat4("LightMVPMatrix",light_mvp);
    phong_shader->setMat4("CameraMVPMatrix",camera_mvp);

    phong_shader->setVec3("light_dir",glm::normalize(glm::vec3{-x,-y,-z}));
    phong_shader->setVec3("camera_pos",camera->getCameraPos());
    phong_shader->setInt("ShadowMap",0);
    phong_shader->setInt("AlbedoMap",1);
    phong_shader->setInt("shadow_mode",shadow.method);
    for(int i = 0;i<models.size();i++){
        glBindTextureUnit(1,models[i].tex);
        GL_EXPR(glBindVertexArray(models[i].vao));
        GL_EXPR(glDrawElements(GL_TRIANGLES,models[i].draw_count,GL_UNSIGNED_INT,0));
        GL_CHECK
    }
    GL_CHECK

}
int main(){
    try
    {
        ShadowMapApplication app;
        app.run();
    }
    catch (const std::exception& err)
    {
        std::cout<<err.what()<<std::endl;
    }
    return 0;
}