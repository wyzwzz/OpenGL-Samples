#include <demo.hpp>
#include <shader_program.hpp>
//#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <logger.hpp>
#include <imgui.h>
#include <array>
#include <fstream>
#include <random>
#include <vector>

using namespace glm;

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
    GL_EXPR(glBindVertexArray(vao));
    vbo = gl->CreateBuffer();
    ebo = gl->CreateBuffer();

    GL_EXPR(glBindBuffer(GL_ARRAY_BUFFER,vbo));
    assert(sphere.positions.size()==sphere.normals.size() && sphere.normals.size() == sphere.uv.size());
    auto vertex_count = sphere.positions.size();
    GL_EXPR(glBufferData(GL_ARRAY_BUFFER,vertex_count*sizeof(float)*8,nullptr,GL_STATIC_DRAW));
    GL_EXPR(glBufferSubData(GL_ARRAY_BUFFER,0,vertex_count*3*sizeof(float),sphere.positions.data()));
    GL_EXPR(glBufferSubData(GL_ARRAY_BUFFER,vertex_count*3*sizeof(float),vertex_count*3*sizeof(float),sphere.normals.data()));
    GL_EXPR(glBufferSubData(GL_ARRAY_BUFFER,vertex_count*6*sizeof(float),vertex_count*2*sizeof(float),sphere.uv.data()));
    GL_EXPR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo));
    GL_EXPR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,index_count*sizeof(uint32_t),sphere.indices.data(),GL_STATIC_DRAW));
    GL_EXPR(glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0));
    GL_EXPR(glEnableVertexAttribArray(0));
    GL_EXPR(glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)(vertex_count*3*sizeof(float))));
    GL_EXPR(glEnableVertexAttribArray(1));
    GL_EXPR(glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)(vertex_count*6*sizeof(float))));
    GL_EXPR(glEnableVertexAttribArray(2));
    GL_EXPR(glBindVertexArray(0));
}

static void LoadTextureFromImageFile(const std::string& filename,GL::GLTexture& tex){

    int w,h,ncomp;
    auto data = stbi_load(filename.c_str(),&w,&h,&ncomp,0);
    if(!data){
        throw std::runtime_error("load image failed: " + filename);
    }
    assert(w > 0 && h > 0 && ncomp > 0);
    GLenum fmt = GL_RGBA;
    if(ncomp == 1){
        fmt = GL_RED;
    }
    else if(ncomp == 2){
        fmt = GL_RG;
    }
    else if(ncomp == 3){
        fmt = GL_RGB;
    }
    else if(ncomp == 4){
        fmt = GL_RGBA;
    }
    GL_EXPR(glBindTexture(GL_TEXTURE_2D,tex));
    GL_EXPR(glTexImage2D(GL_TEXTURE_2D,0,fmt,w,h,0,fmt,GL_UNSIGNED_BYTE,data));
    GL_EXPR(glBindTexture(GL_TEXTURE_2D,0));
    LOG_INFO("load image ok: {}",filename);
}

static void LoadTexture3DFromRaw(const std::string& filename,int x,int y,int z,GL::GLTexture& tex){
    std::ifstream in(filename,std::ios::binary);
    if(!in.is_open()){
        throw std::runtime_error("failed to open raw: " + filename);
    }
    in.seekg(0,std::ios::end);
    int file_size = in.tellg();
    assert(file_size == x * y * z * 4);
    std::vector<unsigned char> data(file_size);
    in.seekg(0,std::ios::beg);
    in.read(reinterpret_cast<char*>(data.data()),file_size);
    in.close();

    GL_EXPR(glBindTexture(GL_TEXTURE_3D,tex));
    GL_EXPR(glTexImage3D(GL_TEXTURE_3D,0,GL_RGBA,x,y,z,0,GL_RGBA,GL_UNSIGNED_BYTE,data.data()));
    GL_EXPR(glBindTexture(GL_TEXTURE_3D,0));
    LOG_INFO("load raw ok: {}",filename);
}

class VolumeCloudApp:public Demo{
  public:
    VolumeCloudApp() = default;
    void initResource() override;
    void render_frame() override ;
    void render_imgui() override;
    ~VolumeCloudApp(){
        LOG_INFO("camera pos: {} {} {}",camera->pos.x,camera->pos.y,camera->pos.z);
    }
  private:
    void process_input() override;

    void loadMesh(const std::string &filename);

    void createGBuffer();

    struct{
        GL::GLVertexArray vao;
        GL::GLBuffer vbo;
        GL::GLBuffer ebo;
        mat4 local_to_world;
        int index_count = 0;
    }terrain;
    std::unique_ptr<Shader> mesh_shader;

    struct{
      GL::GLFramebuffer fbo;
      GL::GLRenderbuffer rbo;
      GL::GLTexture color;
      GL::GLTexture pos;
      GL::GLTexture normal;
      GL::GLTexture depth;
    }gbuffer;

    GL::GLSampler linear_sampler;
    GL::GLSampler nearest_sampler;

    std::unique_ptr<Shader> post_process_shader;

    std::unique_ptr<Shader> cloud_shader;
    GL::GLTexture clound_tex;
    struct{
        GL::GLTexture weather_map;
        GL::GLTexture shape;
        GL::GLSampler shape_sampler;
        GL::GLTexture detail;
        GL::GLSampler detail_sampler;
        GL::GLTexture blue_noise;
    }cloud;
};
void VolumeCloudApp::initResource()
{
    gl->SetVSync(0);

    GL_EXPR(glEnable(GL_DEPTH_TEST));

    camera = std::make_unique<control::FPSCamera>(glm::vec3{4.087f,6.f,16.85f});

    loadMesh("terrain.obj");
    mesh_shader = std::make_unique<Shader>("mesh.vert","mesh.frag");

    createGBuffer();

    linear_sampler = gl->CreateSampler();
    GL_EXPR(glSamplerParameteri(linear_sampler,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE));
    GL_EXPR(glSamplerParameteri(linear_sampler,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE));
    GL_EXPR(glSamplerParameteri(linear_sampler,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE));
    GL_EXPR(glSamplerParameteri(linear_sampler,GL_TEXTURE_MIN_FILTER,GL_LINEAR));
    GL_EXPR(glSamplerParameteri(linear_sampler,GL_TEXTURE_MAG_FILTER,GL_LINEAR));

    nearest_sampler = gl->CreateSampler();
    GL_EXPR(glSamplerParameteri(nearest_sampler,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE));
    GL_EXPR(glSamplerParameteri(nearest_sampler,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE));
    GL_EXPR(glSamplerParameteri(nearest_sampler,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE));
    GL_EXPR(glSamplerParameteri(nearest_sampler,GL_TEXTURE_MIN_FILTER,GL_NEAREST));
    GL_EXPR(glSamplerParameteri(nearest_sampler,GL_TEXTURE_MAG_FILTER,GL_NEAREST));


    post_process_shader = std::make_unique<Shader>("quad.vert","post_process.frag");
    post_process_shader->use();
    post_process_shader->setInt("GColor",0);

    cloud_shader = std::make_unique<Shader>("quad.vert","cloud.frag");
    cloud_shader->use();
    cloud_shader->setInt("GDepth",0);
    cloud_shader->setInt("GColor",1);
    cloud_shader->setInt("WeatherMap",2);
    cloud_shader->setInt("ShapeNoise",3);
    cloud_shader->setInt("DetailNoise",4);
    cloud_shader->setInt("BlueNoise",5);

    clound_tex = gl->CreateTexture(GL_TEXTURE_2D);
    GL_EXPR(glTextureStorage2D(clound_tex,1,GL_RGBA8,window_w,window_h));

    cloud.weather_map = gl->CreateTexture(GL_TEXTURE_2D);
    LoadTextureFromImageFile("weather.png",cloud.weather_map);

    cloud.shape = gl->CreateTexture(GL_TEXTURE_3D);
    LoadTexture3DFromRaw("noiseShapePacked_128_128_128.raw",128,128,128,cloud.shape);
    cloud.shape_sampler = gl->CreateSampler();
    GL_EXPR(glSamplerParameteri(cloud.shape_sampler,GL_TEXTURE_WRAP_S,GL_REPEAT));
    GL_EXPR(glSamplerParameteri(cloud.shape_sampler,GL_TEXTURE_WRAP_T,GL_REPEAT));
    GL_EXPR(glSamplerParameteri(cloud.shape_sampler,GL_TEXTURE_WRAP_R,GL_REPEAT));
    GL_EXPR(glSamplerParameteri(cloud.shape_sampler,GL_TEXTURE_MIN_FILTER,GL_LINEAR));
    GL_EXPR(glSamplerParameteri(cloud.shape_sampler,GL_TEXTURE_MAG_FILTER,GL_LINEAR));

    cloud.detail_sampler = gl->CreateSampler();
    GL_EXPR(glSamplerParameteri(cloud.detail_sampler,GL_TEXTURE_WRAP_S,GL_REPEAT));
    GL_EXPR(glSamplerParameteri(cloud.detail_sampler,GL_TEXTURE_WRAP_T,GL_REPEAT));
    GL_EXPR(glSamplerParameteri(cloud.detail_sampler,GL_TEXTURE_WRAP_R,GL_REPEAT));
    GL_EXPR(glSamplerParameteri(cloud.detail_sampler,GL_TEXTURE_MIN_FILTER,GL_LINEAR));
    GL_EXPR(glSamplerParameteri(cloud.detail_sampler,GL_TEXTURE_MAG_FILTER,GL_LINEAR));

    cloud.blue_noise = gl->CreateTexture(GL_TEXTURE_2D);
    LoadTextureFromImageFile("blueNoise.png",cloud.blue_noise);
}
inline float deg2rad(float deg){
    return deg / 180.f * pi<float>();
}
void VolumeCloudApp::createGBuffer()
{
    gbuffer.fbo = gl->CreateFramebuffer();
    GL_EXPR(glBindFramebuffer(GL_FRAMEBUFFER,gbuffer.fbo));
    gbuffer.rbo = gl->CreateRenderbuffer();
    GL_EXPR(glBindRenderbuffer(GL_RENDERBUFFER,gbuffer.rbo));
    GL_EXPR(glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH32F_STENCIL8,window_w,window_h));
    GL_EXPR(glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,gbuffer.rbo));
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){
        std::cerr<<std::hex<<glCheckFramebufferStatus(GL_FRAMEBUFFER)<<std::endl;
        throw std::runtime_error("Framebuffer object is not complete!");
    }

    gbuffer.color = gl->CreateTexture(GL_TEXTURE_2D);
    GL_EXPR(glTextureStorage2D(gbuffer.color,1,GL_RGBA8,window_w,window_h));

    gbuffer.pos = gl->CreateTexture(GL_TEXTURE_2D);
    GL_EXPR(glTextureStorage2D(gbuffer.pos,1,GL_RGBA32F,window_w,window_h));

    gbuffer.normal = gl->CreateTexture(GL_TEXTURE_2D);
    GL_EXPR(glTextureStorage2D(gbuffer.normal,1,GL_RGBA8,window_w,window_h));

    gbuffer.depth = gl->CreateTexture(GL_TEXTURE_2D);
    GL_EXPR(glTextureStorage2D(gbuffer.depth,1,GL_R32F,window_w,window_h));

}

void VolumeCloudApp::render_frame()
{
    GL_EXPR(glClearColor(0.0,0.0,0.0,0.0));

    auto camera_view = camera->getViewMatrix();
    auto camera_proj = perspective(deg2rad(camera->getZoom()),window_w * 1.f / window_h,0.1f,50.f);

    GL_EXPR(glBindFramebuffer(GL_FRAMEBUFFER,gbuffer.fbo));
//    GL_EXPR(glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT));

    // render mesh
    mesh_shader->use();
    mesh_shader->setMat4("Model",terrain.local_to_world);
    mesh_shader->setMat4("VP",camera_proj * camera_view);
    mesh_shader->setMat4("NormalModel", transpose(inverse(terrain.local_to_world)));
    GL_EXPR(glBindVertexArray(terrain.vao));
    GL_EXPR(glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,gbuffer.color,0));
    GL_EXPR(glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT1,GL_TEXTURE_2D,gbuffer.depth,0));
    static GLenum mesh_draw_buffers[] = {GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1};
    static constexpr unsigned char black[] = {54,87,135,0};
    static constexpr float red[] = {1.f};
    GL_EXPR(glClearTexImage(gbuffer.color,0,GL_RGBA,GL_UNSIGNED_BYTE,black));
    GL_EXPR(glClearTexImage(gbuffer.depth,0,GL_RED,GL_FLOAT,red));
    GL_EXPR(glDrawBuffers(2,mesh_draw_buffers));
    GL_EXPR(glClear(GL_DEPTH_BUFFER_BIT));
    GL_EXPR(glDrawElements(GL_TRIANGLES, terrain.index_count, GL_UNSIGNED_INT, nullptr));

    // render cloud
    cloud_shader->use();
    cloud_shader->setMat4("InvView",inverse(camera_view));
    cloud_shader->setMat4("InvProj",inverse(camera_proj));
    cloud_shader->setVec3("CameraPos",camera->getCameraPos());
    cloud_shader->setVec3("LightDir", normalize(vec3(0,-0.5,0.5)));
    cloud_shader->setVec3("LightRadiance",10.f,10.f,10.f);

    GL_EXPR(glBindTextureUnit(0,gbuffer.depth));
    GL_EXPR(glBindTextureUnit(1,gbuffer.color));
    GL_EXPR(glBindTextureUnit(2,cloud.weather_map));
    GL_EXPR(glBindTextureUnit(3,cloud.shape));
//    GL_EXPR(glBindTextureUnit(4,cloud.detail));
    GL_EXPR(glBindTextureUnit(5,cloud.blue_noise));
    GL_EXPR(glBindSampler(0,nearest_sampler));
    GL_EXPR(glBindSampler(1,nearest_sampler));
    GL_EXPR(glBindSampler(2,linear_sampler));
    GL_EXPR(glBindSampler(3,cloud.shape_sampler));
    GL_EXPR(glBindSampler(5,nearest_sampler));
    GL_EXPR(glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,clound_tex,0));
    GL_EXPR(glDrawBuffer(GL_COLOR_ATTACHMENT0));
    GL_EXPR(glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT));
    GL_EXPR(glDrawArrays(GL_TRIANGLE_STRIP,0,4));


    // post process
    GL_EXPR(glBindFramebuffer(GL_FRAMEBUFFER,0));
    GL_EXPR(glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT));

    post_process_shader->use();
    GL_EXPR(glBindTextureUnit(0,clound_tex));
    GL_EXPR(glBindSampler(0,nearest_sampler));
    GL_EXPR(glDrawArrays(GL_TRIANGLE_STRIP,0,4));

}
void VolumeCloudApp::render_imgui()
{
    Demo::render_imgui();
}
void VolumeCloudApp::process_input()
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
void VolumeCloudApp::loadMesh(const std::string &filename)
{
    auto mesh = load_mesh_from_obj(filename);
    terrain.local_to_world = translate(mat4(1.f),{0.f,1.f,0.f});
    terrain.index_count = mesh.indices.size();
    terrain.vao = gl->CreateVertexArray();
    GL_EXPR(glBindVertexArray(terrain.vao));
    terrain.vbo = gl->CreateBuffer();
    GL_EXPR(glBindBuffer(GL_ARRAY_BUFFER,terrain.vbo));
    GL_EXPR(glBufferData(GL_ARRAY_BUFFER,mesh.vertices.size() * sizeof(vertex_t),mesh.vertices.data(),GL_STATIC_DRAW));
    terrain.ebo = gl->CreateBuffer();
    GL_EXPR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,terrain.ebo));
    GL_EXPR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,mesh.indices.size() * sizeof(uint32),mesh.indices.data(),GL_STATIC_DRAW));
    GL_EXPR(glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8 * sizeof(float),(void*)0));
    GL_EXPR(glEnableVertexAttribArray(0));
    GL_EXPR(glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8 * sizeof(float),(void*)(3 * sizeof(float))));
    GL_EXPR(glEnableVertexAttribArray(1));
    GL_EXPR(glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8 * sizeof(float),(void*)(6 * sizeof(float))));
    GL_EXPR(glEnableVertexAttribArray(2));
    GL_EXPR(glBindVertexArray(0));
}

int main(){
    try
    {
        VolumeCloudApp app;
        app.run();
    }
    catch (const std::exception& err)
    {
        std::cerr<<err.what()<<std::endl;
    }
    return 0;
}