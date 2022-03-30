#include <shader_program.hpp>
#include <demo.hpp>
#include <mesh.hpp>
#include <logger.hpp>
#include <array>
#include <fstream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <imgui.h>

const std::string AssetPath = "C:/Users/wyz/projects/OpenGL-Samples/examples/PRT/precompute/prt/scenes/";
const std::string ShaderPath = "C:/Users/wyz/projects/OpenGL-Samples/examples/PRT/renderer/shader/";
static void CreateCube(const std::shared_ptr<GL>& gl,GL::GLVertexArray& vao,GL::GLBuffer& vbo){
            float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        
        vao = gl->CreateVertexArray();
        glBindVertexArray(vao);
        vbo = gl->CreateBuffer();
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        GL_CHECK
}

class PRTApplication final:public Demo{
    void initResource() override;
    void process_input() override;
    void render_frame() override;
    void render_imgui() override;

    struct{
        GL::GLVertexArray vao;
        GL::GLBuffer vbo;
        uint32_t triangle_count{0};
    }draw_model;

    std::string CubeMap[4]={"CornellBox","GraceCathedral","Indoor","Skybox"};
    int cube_map_idx{0};
    struct{
        GL::GLVertexArray vao;
        GL::GLBuffer vbo;
        GL::GLTexture env_cube;
        std::unique_ptr<Shader> skybox_shader;
        std::string name;
    }cube_map;

    struct{
        static constexpr int SHOrder = 2;
        static constexpr int SHCoeffLength =(SHOrder+1)*(SHOrder+1);//9
        std::array<float,SHCoeffLength> light[3];//3 for rgb
        //must padding(resize) to alignment of 16 bytes
        std::vector<std::array<float,SHCoeffLength>> light_transport;//size = vertex count
        GL::GLBuffer light_transport_ssbo;

        std::unique_ptr<Shader> prt_shader;
    }prt;

    void loadPRTResource(){
        std::ifstream f_light(AssetPath+"cubemap/"+cube_map.name+"/light.txt");
        if(!f_light.is_open()){
            throw std::runtime_error("open light.txt failed");
        }
        for(int i = 0; i < prt.SHCoeffLength; i++){
            f_light >> prt.light[0][i] >> prt.light[1][i] >>prt.light[2][i]; 
        }
        for(int i = 0; i < prt.SHCoeffLength; i++){
            LOG_INFO("prt light {} rgb: {} {} {}",i,prt.light[0][i],prt.light[1][i],prt.light[2][i]);
        }
        f_light.close();

        std::ifstream f_light_transport(AssetPath+"cubemap/"+cube_map.name+"/transport.txt");
        if(!f_light_transport.is_open()){
            throw std::runtime_error("open transport.txt failed");
        }
        int vertex_count;
        f_light_transport >> vertex_count;
        size_t triangle_v_count = draw_model.triangle_count * 3;
        prt.light_transport.resize(triangle_v_count);
        for(int i = 0; i < triangle_v_count; i++){
            for(int j = 0; j < prt.SHCoeffLength; j++){
                f_light_transport >> prt.light_transport[i][j];
            }
        }
        f_light_transport.close();

        if(prt.light_transport_ssbo){
            prt.light_transport_ssbo.Release();
        }
        prt.light_transport_ssbo = gl->CreateBuffer();
        glBindBuffer(GL_SHADER_STORAGE_BUFFER,prt.light_transport_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER,prt.light_transport.size()*sizeof(prt.light_transport[0]),prt.light_transport.data(),GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,prt.light_transport_ssbo);
        if(!prt.prt_shader){
            prt.prt_shader = std::make_unique<Shader>(
                (ShaderPath+"prt_v.glsl").c_str(),(ShaderPath+"prt_f.glsl").c_str()
            );
        }
        prt.prt_shader->use();
        for(int i = 0;i<prt.SHCoeffLength;i++){
            prt.prt_shader->setVec3("prtLight["+std::to_string(i)+"]",prt.light[0][i],prt.light[1][i],prt.light[2][i]);
        }
        GL_CHECK
    }

    void loadDrawModel(){
        auto triangles = load_triangles_from_obj(AssetPath+"mary.obj");
        LOG_INFO("triangles count: {}",triangles.size());
        draw_model.triangle_count = triangles.size();
        draw_model.vao = gl->CreateVertexArray();
        glBindVertexArray(draw_model.vao);
        draw_model.vbo = gl->CreateBuffer();
        glBindBuffer(GL_ARRAY_BUFFER,draw_model.vbo);
        glBufferData(GL_ARRAY_BUFFER,sizeof(triangle_t)*triangles.size(),triangles.data(),GL_STATIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(0));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);

    }
    void loadCubeMap(const std::string& name){
        static std::string dir[6] = {
            "posx","negx","posy","negy","posz","negz"
        };
        glBindTexture(GL_TEXTURE_CUBE_MAP,cube_map.env_cube);
        for(int i = 0; i < 6; i++){
            int width,height,nrComponents;
            auto path = AssetPath + "cubemap/"+name+"/"+dir[i]+".jpg";
            auto data = stbi_load(path.c_str(),&width,&height,&nrComponents,0);
            if(data == nullptr){
                LOG_ERROR("load {} failed!",path);
            }
            if(nrComponents!=3){
                throw std::runtime_error("invaild format");
            }
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,0,GL_RGB8,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,data);
        }
    }
    void loadEnvironmentMap(){
        CreateCube(gl,cube_map.vao,cube_map.vbo);
        cube_map.env_cube = gl->CreateTexture(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP,cube_map.env_cube);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // be sure to set minification filter to mip_linear 
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // loadCubeMap("Indoor");

        cube_map.skybox_shader = std::make_unique<Shader>(
            (ShaderPath+"background_v.glsl").c_str(),
            (ShaderPath+"background_f.glsl").c_str()
        );
        cube_map.skybox_shader->use();
        cube_map.skybox_shader->setInt("environmentMap",0);
        glDepthFunc(GL_LEQUAL);
        // glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        GL_CHECK
    }

};

void PRTApplication::initResource(){
    gl->SetWindowTitle("PRT");

    glEnable(GL_DEPTH_TEST);

    loadEnvironmentMap();

    loadDrawModel();

    camera = std::make_unique<control::FPSCamera>(glm::vec3{0.f,0.f,3.f});
}

void PRTApplication::process_input(){
    static auto t = glfwGetTime();
    auto cur_t = glfwGetTime();
    auto delta_t = cur_t - t;
    t = cur_t;

    delta_t *= 0.2;

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

void PRTApplication::render_frame(){
    if(CubeMap[cube_map_idx]!=cube_map.name || cube_map.name.empty()){
        cube_map.name = CubeMap[cube_map_idx];
        loadCubeMap(cube_map.name);
        loadPRTResource();
    }

    glClearColor(0.f,0.f,0.f,0.f);
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

    auto model = glm::mat4(1.f);
    auto view = camera->getViewMatrix();
    auto proj = glm::perspective(glm::radians(camera->getZoom()),(float)gl->Width()/gl->Height(),0.01f,20.f);

    prt.prt_shader->use();
    prt.prt_shader->setMat4("model",model);
    prt.prt_shader->setMat4("view",view);
    prt.prt_shader->setMat4("projection",proj);
    glBindVertexArray(draw_model.vao);
    glDrawArrays(GL_TRIANGLES,0,draw_model.triangle_count*3);

    cube_map.skybox_shader->use();
    cube_map.skybox_shader->setMat4("view",view);
    cube_map.skybox_shader->setMat4("projection",proj);
    glBindTextureUnit(0,cube_map.env_cube);
    glBindVertexArray(cube_map.vao);
    glDrawArrays(GL_TRIANGLES,0,36);

    GL_CHECK
}

void PRTApplication::render_imgui(){
    ImGui::Text("PRT");
    ImGui::Text("CubeMap");
    ImGui::RadioButton("CornellBox",&cube_map_idx,0);
    ImGui::RadioButton("GraceCathedral",&cube_map_idx,1);
    ImGui::RadioButton("Indoor",&cube_map_idx,2);
    ImGui::RadioButton("Skybox",&cube_map_idx,3);

}

int main(){
    try{
        PRTApplication app;
        app.run();
    }
    catch(const std::exception& err){
        LOG_ERROR("{}",err.what());
    }
    return 0;
}
