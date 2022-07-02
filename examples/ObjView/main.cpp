//
// Created by wyz on 2022/7/6.
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
static Sphere MakeSphere(int seg_count = 64){
    const uint32_t X_SEGMENTS = seg_count;
    const uint32_t Y_SEGMENTS = seg_count;
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
static void CreateSphere(const std::shared_ptr<GL>& gl,GL::GLVertexArray& vao,GL::GLBuffer& vbo,GL::GLBuffer& ebo,int& index_count,int seg_count = 64){
    auto sphere = MakeSphere(seg_count);
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


class ObjView:public Demo{
  public:
    ObjView() = default;
    void initResource() override;
    void render_frame() override ;
    void render_imgui() override;
  private:
    void process_input() override;


    struct{
        GL::GLVertexArray vao;
        GL::GLBuffer vbo;
        GL::GLBuffer ebo;
        int index_count = 0;
        int seg_count = 64;
        float radius = 0.1f;
    }sphere;
    struct{
        float x_degree = 0;
        float y_degree = 45;
    }light;
    std::unique_ptr<Shader> shader;
    std::vector<vec3f> pos;
    std::unique_ptr<control::TrackBallCamera> cam;
};
std::vector<vec3f> ReadPosFromFile(const std::string& filename){
    std::ifstream in(filename);
    if(!in.is_open()){
        throw std::runtime_error("open file failed");
    }
    std::vector<vec3f> pos;
    for(std::string line; std::getline(in,line);){
        vec3f p(0);
        sscanf(line.substr(1).c_str(),"%f %f %f\n",&p.x,&p.y,&p.z);
        pos.emplace_back(p);
    }
    in.close();
    return pos;
}
void ObjView::initResource()
{
    gl->SetVSync(0);

    GL_EXPR(glEnable(GL_DEPTH_TEST));

//    camera = std::make_unique<control::FPSCamera>(glm::vec3{0.f,0.f,3.957f});
    cam = std::make_unique<control::TrackBallCamera>(4,window_w,window_h,vec3f(0));
    GL::MouseEvent = [&](void *, MouseButton buttons, EventAction action, int xpos, int ypos) {
        static bool left_button_press;
        if (buttons == Mouse_Left && action == Press)
        {
            left_button_press = true;
            cam->processMouseButton(control::CameraDefinedMouseButton::Left, true, xpos, ypos);
        }
        else if (buttons == Mouse_Left && action == Release)
        {
            left_button_press = false;
            cam->processMouseButton(control::CameraDefinedMouseButton::Left, false, xpos, ypos);
        }
        else if (action == Move && buttons == Mouse_Left && left_button_press)
        {
            cam->processMouseMove(xpos, ypos);
        }
    };
    GL::ScrollEvent = [&](void *, int x, int y) { cam->processMouseScroll(y * 0.1); };
    pos = ReadPosFromFile("neuron_position.obj");

    CreateSphere(gl,sphere.vao,sphere.vbo,sphere.ebo,sphere.index_count);

    shader = std::make_unique<Shader>("mesh.vert","mesh.frag");
    shader->use();
    shader->setVec3("Color",vec3f(1));
}
void ObjView::render_frame()
{
    GL_EXPR(glClearColor(0,0,0,0));
    GL_EXPR(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));


    auto view = cam->getViewMatrix();
    auto proj = glm::perspective(glm::radians(cam->getZoom()),(float)window_w/window_h,0.1f,20.f);

    auto light_dir_y = std::sin(glm::radians(light.y_degree));
    auto light_dir_x = std::cos(glm::radians(light.y_degree)) * std::cos(glm::radians(light.x_degree));
    auto light_dir_z = std::cos(glm::radians(light.y_degree)) * std::sin(glm::radians(light.x_degree));

    shader->use();
    shader->setMat4("vp",proj * view);
    shader->setVec3("LightDir",-vec3f(light_dir_x,light_dir_y,light_dir_z));
    GL_EXPR(glBindVertexArray(sphere.vao));

    for(auto& p:pos){
        auto s = glm::scale(glm::mat4(1),vec3f(sphere.radius));
        auto t = glm::translate(glm::mat4(1),vec3f(p.x,p.y,p.z));
        auto model = t * s;
        shader->setMat4("model",model);

        GL_EXPR(glDrawElements(GL_TRIANGLE_STRIP,sphere.index_count,GL_UNSIGNED_INT,nullptr));
    }


}
void ObjView::render_imgui()
{
    ImGui::Begin("Settings");
    ImGui::Text("FPS: %.0f",ImGui::GetIO().Framerate);
    ImGui::InputFloat("Sphere Radius",&sphere.radius,0.01);
    ImGui::SliderFloat("Light X Degree",&light.x_degree,0,360);
    ImGui::SliderFloat("Light Y Degree",&light.y_degree,0,90);
    ImGui::End();
}
void ObjView::process_input()
{
    static auto t = glfwGetTime();
    auto cur_t = glfwGetTime();
    auto delta_t = cur_t - t;
    t = cur_t;

    if(glfwGetKey(gl->GetWindow(),GLFW_KEY_W))
        cam->processKeyEvent(control::CameraDefinedKey::Forward,delta_t);
    if(glfwGetKey(gl->GetWindow(),GLFW_KEY_S))
        cam->processKeyEvent(control::CameraDefinedKey::Backward,delta_t);
    if(glfwGetKey(gl->GetWindow(),GLFW_KEY_A))
        cam->processKeyEvent(control::CameraDefinedKey::Left,delta_t);
    if(glfwGetKey(gl->GetWindow(),GLFW_KEY_D))
        cam->processKeyEvent(control::CameraDefinedKey::Right,delta_t);
    if(glfwGetKey(gl->GetWindow(),GLFW_KEY_Q))
        cam->processKeyEvent(control::CameraDefinedKey::Up,delta_t);
    if(glfwGetKey(gl->GetWindow(),GLFW_KEY_E))
        cam->processKeyEvent(control::CameraDefinedKey::Bottom,delta_t);
}

int main(){
    try
    {
        ObjView app;
        app.run();
    }
    catch (const std::exception& err)
    {
        std::cout<<err.what()<<std::endl;
    }
    return 0;
}