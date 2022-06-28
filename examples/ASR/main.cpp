#include <demo.hpp>
#include <shader_program.hpp>
//#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <logger.hpp>
#include <imgui.h>
#include <array>

#include "atmosphere.hpp"
#include <random>

#include <cyPoint.h>
#include <cySampleElim.h>
using namespace glm;
class ASRApplication: public Demo{
  public:
    ASRApplication() = default;
    void initResource() override;
    void render_frame() override ;
    void render_imgui() override;
  private:
    void process_input() override;

    void generateTransmittanceLUT();

    void generateMultiScatteringLUT();

    void loadTerrainMesh(const std::string& filename);

    void setupShadowMap();

    void setupSkyLUT();

    void setupAerialLUT();

    void setupSkyView();

    void generateShadowMap(const mat4& vp);

    void generateSkyLUT(const vec3f& sun_dir,const vec3f& sun_radiance);

    void generateAerialLUT(const vec3f& sun_dir,const mat4& sun_vp);

    void updateFrustumDirs(const mat4& inv_view_proj);

    void renderMeshes(const vec3f& sun_dir,const vec3f& sun_radiance,const mat4& sun_vp);

    void renderSky();

    void renderSunDisk(const vec3f& sun_dir,const vec3f& sun_radiance);
  private:

    vec2i transmittance_lut_size = {256,256};
    vec2i multi_scattering_lut_size = {256,256};
    vec2i sky_lut_size = {64,64};
    vec3i aerial_lut_size = {200,150,32};
    float ground_albedo = 0.3f;
    float world_scale = 200.f;

    bool enable_terrain = true;
    bool enable_sky = true;
    bool enable_shadow = true;
    bool enable_sun_disk = true;
    bool enalbe_multiscattering = true;

    struct{
        float sun_x_degree = 0.f;
        float sun_y_degree = 11.6f;
        float sun_intensity = 10.f;
        vec3f sun_color = {1.f,1.f,1.f};
    };

    AtmosphereProperties preset_atmosphere_properties;
    AtmosphereProperties std_unit_atmosphere_properties;

    GL::GLBuffer atmosphere_buffer;

    std::unique_ptr<Shader> transmittance_shader;
    GL::GLTexture transmittance_lut;

    struct MultiScatteringParams{
        vec3f ground_albedo = vec3f(0.3);
        int dir_sample_count = 64;

        vec3f sun_intensity = vec3f(1);
        int ray_march_step_count = 256;
    }multi_scattering_params;

    std::unique_ptr<Shader> multiscattering_shader;
    GL::GLBuffer multiscattering_buffer;
    GL::GLBuffer random_samples_sbuffer;
    GL::GLTexture multiscattering_lut;

    GL::GLSampler lut_sampler;

    std::unique_ptr<Shader> mesh_shader;
    struct{
        GL::GLVertexArray vao;
        GL::GLBuffer vbo;
        GL::GLBuffer ebo;
        mat4 local_to_world;
        int index_count = 0;
    }terrain;

    std::unique_ptr<Shader> shadow_shader;
    struct{
        GL::GLFramebuffer fbo;
        GL::GLRenderbuffer rbo;
        GL::GLTexture shadow_tex;
        GL::GLSampler sampler;
    }shadow;
    constexpr static vec2i ShadowMapSize = {2048,2048};


    struct SkyParams{
        vec3f view_pos;
        int ray_march_step_count = 40;
        vec3f sun_dir;
        int enable_multiscattering = 1;
        vec3f sun_intensity;
        float padding = 0;
    }sky_params;
    static_assert(sizeof(SkyParams) == 48,"");
    std::unique_ptr<Shader> sky_lut_shader;
    struct{
        GL::GLFramebuffer fbo;
        GL::GLRenderbuffer rbo;
    }sky_lut_frame;
    GL::GLTexture sky_lut;
    GL::GLBuffer sky_buffer;

    struct FrustumDirections{
        vec3f frustum_a;
        vec3f frustum_b;
        vec3f frustum_c;
        vec3f frustum_d;
    }frustum_dirs;

    struct AerialParams{
        vec3f sun_dir;
        float sun_theta;
        vec3f frustum_a;
        float max_view_distance;
        vec3f frustum_b;
        int ray_march_step_count_per_slice;
        vec3f frustum_c;
        int enable_multiscattering;
        vec3f frustum_d;
        float view_height;
        vec3f view_pos;
        int enable_shadow;
        mat4 shadow_vp;
        float world_scale;
        float padding[3];
    }aerial_params;
    static_assert(sizeof(AerialParams) == 176,"");
    std::unique_ptr<Shader> aerial_lut_shader;
    GL::GLTexture aerial_lut;
    GL::GLBuffer aerial_buffer;


    struct SkyViewParams{
        vec3f frustum_a; float pad0;
        vec3f frustum_b; float pad1;
        vec3f frustum_c; float pad2;
        vec3f frustum_d; float pad3;
    }sky_view_params;
    static_assert(sizeof(SkyViewParams) == 64,"");
    std::unique_ptr<Shader> sky_shader;
    GL::GLBuffer sky_view_buffer;


    std::unique_ptr<Shader> sun_shader;
    std::unique_ptr<Shader> postprocess_shader;
};
void ASRApplication::initResource()
{
    gl->SetVSync(0);

    camera = std::make_unique<control::FPSCamera>(glm::vec3{4.087f,3.7f,3.957f});

    // 0. create some common resources
    std_unit_atmosphere_properties = preset_atmosphere_properties.toStdUnit();
    atmosphere_buffer = gl->CreateBuffer();
    GL_EXPR(glNamedBufferData(atmosphere_buffer,sizeof(AtmosphereProperties),&std_unit_atmosphere_properties,GL_STATIC_READ));

    lut_sampler = gl->CreateSampler();
    GL_EXPR(glSamplerParameteri(lut_sampler,GL_TEXTURE_MIN_FILTER,GL_LINEAR));
    GL_EXPR(glSamplerParameteri(lut_sampler,GL_TEXTURE_MAG_FILTER,GL_LINEAR));
    GL_EXPR(glSamplerParameteri(lut_sampler,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE));
    GL_EXPR(glSamplerParameteri(lut_sampler,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE));
    GL_EXPR(glSamplerParameteri(lut_sampler,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE));

    // 1. generate atmosphere transmittance look-up table in Texture2D
    generateTransmittanceLUT();
    LOG_INFO("generate transmittance lut ok");
    // 2.
    generateMultiScatteringLUT();
    LOG_INFO("generate multiscatttering lut ok");

    // 3.
    loadTerrainMesh("terrain.obj");

    // 4. shadow map texture
    setupShadowMap();
    // 5. sky lut
    setupSkyLUT();
    // 6. aerial lut
    setupAerialLUT();

    //

    //
    setupSkyView();
}
void ASRApplication::generateTransmittanceLUT()
{
    if(!transmittance_shader){
        transmittance_shader = std::make_unique<Shader>("transmittance.comp");
    }

    //create transmittance lut texture image
    transmittance_lut = gl->CreateTexture(GL_TEXTURE_2D);
    GL_EXPR(glTextureStorage2D(transmittance_lut,1,GL_RGBA32F,transmittance_lut_size.x,transmittance_lut_size.y));
    GL_EXPR(glBindImageTexture(0,transmittance_lut,0,GL_FALSE,0,GL_READ_WRITE,GL_RGBA32F));

    GL_EXPR(glBindBufferBase(GL_UNIFORM_BUFFER,0,atmosphere_buffer));

    transmittance_shader->use();


    constexpr int group_thread_size_x = 16;
    constexpr int group_thread_size_y = 16;
    const int group_size_x = (transmittance_lut_size.x + group_thread_size_x - 1) / group_thread_size_x;
    const int group_size_y = (transmittance_lut_size.y + group_thread_size_y - 1) / group_thread_size_y;
    GL_EXPR(glDispatchCompute(group_size_x,group_size_y,1));
    //cpu code note https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glMemoryBarrier.xhtml
    GL_EXPR(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));
}

std::vector<vec2f> getPoissonDiskSamples(int count){
    std::default_random_engine rng{ std::random_device()() };
    std::uniform_real_distribution<float> dis(0, 1);

    std::vector<cy::Point2f> rawPoints;
    for(int i = 0; i < count * 10; ++i)
    {
        const float u = dis(rng);
        const float v = dis(rng);
        rawPoints.push_back({ u, v });
    }

    std::vector<cy::Point2f> outputPoints(count);

    cy::WeightedSampleElimination<cy::Point2f, float, 2> wse;
    wse.SetTiling(true);
    wse.Eliminate(
        rawPoints.data(),    rawPoints.size(),
        outputPoints.data(), outputPoints.size());

    std::vector<vec2f> result;
    for(auto &p : outputPoints)
        result.push_back({ p.x, p.y });

    return result;
}

void ASRApplication::generateMultiScatteringLUT()
{
    if(!multiscattering_shader){
        multiscattering_shader = std::make_unique<Shader>("multiscattering.comp");
    }
    multiscattering_shader->use();
    // bind atmosphere properties uniform buffer
    GL_EXPR(glBindBufferBase(GL_UNIFORM_BUFFER,0,atmosphere_buffer));
    //create multiscattering lut texture image
    multiscattering_lut = gl->CreateTexture(GL_TEXTURE_2D);
    GL_EXPR(glTextureStorage2D(multiscattering_lut,1,GL_RGBA32F,multi_scattering_lut_size.x,multi_scattering_lut_size.y));
    GL_EXPR(glBindImageTexture(0,multiscattering_lut,0,GL_FALSE,0,GL_READ_WRITE,GL_RGBA32F));
    //bind transmittance lut
    multiscattering_shader->setInt("Transmittance",0);
    GL_EXPR(glBindTextureUnit(0,transmittance_lut));
    GL_EXPR(glBindSampler(0,lut_sampler));
    //create multiscattering params uniform buffer
    multiscattering_buffer = gl->CreateBuffer();
    GL_EXPR(glNamedBufferData(multiscattering_buffer,sizeof(MultiScatteringParams),&multi_scattering_params,GL_STATIC_DRAW));
    GL_EXPR(glBindBufferBase(GL_UNIFORM_BUFFER,1,multiscattering_buffer));
    //create random samples storage buffer
    std::vector<vec2f> samples = getPoissonDiskSamples(multi_scattering_params.dir_sample_count);
    random_samples_sbuffer = gl->CreateBuffer();
    GL_EXPR(glNamedBufferData(random_samples_sbuffer,samples.size() * sizeof(vec2f),samples.data(),GL_STATIC_DRAW));
    GL_EXPR(glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,random_samples_sbuffer));

    constexpr int group_thread_size_x = 16;
    constexpr int group_thread_size_y = 16;
    const int group_size_x = (multi_scattering_lut_size.x + group_thread_size_x - 1) / group_thread_size_x;
    const int group_size_y = (multi_scattering_lut_size.y + group_thread_size_y - 1) / group_thread_size_y;
    GL_EXPR(glDispatchCompute(group_size_x,group_size_y,1));
    GL_EXPR(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));
}

void ASRApplication::loadTerrainMesh(const std::string &filename)
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
void ASRApplication::setupShadowMap()
{
    shadow_shader = std::make_unique<Shader>("mesh.vert","shadow.frag");
    shadow.shadow_tex = gl->CreateTexture(GL_TEXTURE_2D);
    GL_EXPR(glTextureStorage2D(shadow.shadow_tex,1,GL_R32F,ShadowMapSize.x,ShadowMapSize.y));
    shadow.sampler = gl->CreateSampler();
    GL_EXPR(glSamplerParameteri(shadow.sampler,GL_TEXTURE_MIN_FILTER,GL_NEAREST));
    GL_EXPR(glSamplerParameteri(shadow.sampler,GL_TEXTURE_MIN_FILTER,GL_NEAREST));
    GL_EXPR(glSamplerParameteri(shadow.sampler,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE));
    GL_EXPR(glSamplerParameteri(shadow.sampler,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE));
    GL_EXPR(glSamplerParameteri(shadow.sampler,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE));

    shadow.fbo = gl->CreateFramebuffer();
    GL_EXPR(glBindFramebuffer(GL_FRAMEBUFFER,shadow.fbo));
    shadow.rbo = gl->CreateRenderbuffer();
    GL_EXPR(glBindRenderbuffer(GL_RENDERBUFFER,shadow.rbo));
    GL_EXPR(glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH32F_STENCIL8,ShadowMapSize.x,ShadowMapSize.y));
    GL_EXPR(glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,shadow.rbo));
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){
        std::cerr<<std::hex<<glCheckFramebufferStatus(GL_FRAMEBUFFER)<<std::endl;
        throw std::runtime_error("Framebuffer object is not complete!");
    }
    GL_EXPR(glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,shadow.shadow_tex,0));

    GL_EXPR(glBindFramebuffer(GL_FRAMEBUFFER,0));
}
void ASRApplication::setupSkyLUT()
{
    sky_lut_shader = std::make_unique<Shader>("quad.vert","sky_lut.frag");
    sky_lut_shader->use();
    sky_lut_shader->setInt("Transmittance",0);
    sky_lut_shader->setInt("MultiScattering",1);

    sky_lut = gl->CreateTexture(GL_TEXTURE_2D);
    GL_EXPR(glTextureStorage2D(sky_lut,1,GL_RGBA32F,sky_lut_size.x,sky_lut_size.y));

    sky_buffer = gl->CreateBuffer();
    GL_EXPR(glNamedBufferData(sky_buffer,sizeof(SkyParams),&sky_params,GL_STATIC_DRAW));

    sky_lut_frame.fbo = gl->CreateFramebuffer();
    GL_EXPR(glBindFramebuffer(GL_FRAMEBUFFER,sky_lut_frame.fbo));
    sky_lut_frame.rbo = gl->CreateRenderbuffer();
    GL_EXPR(glBindRenderbuffer(GL_RENDERBUFFER,sky_lut_frame.rbo));
    GL_EXPR(glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH24_STENCIL8,sky_lut_size.x,sky_lut_size.y));
    GL_EXPR(glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,sky_lut_frame.rbo));
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){
        std::cerr<<std::hex<<glCheckFramebufferStatus(GL_FRAMEBUFFER)<<std::endl;
        throw std::runtime_error("Framebuffer object is not complete!");
    }
    GL_EXPR(glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,sky_lut,0));
    GL_EXPR(glBindFramebuffer(GL_FRAMEBUFFER,0));
}
void ASRApplication::setupAerialLUT()
{
    if(!aerial_lut_shader){
        aerial_lut_shader = std::make_unique<Shader>("aerial_lut.comp");
    }
    aerial_lut_shader->use();
    aerial_lut_shader->setInt("Transmittance",0);
    aerial_lut_shader->setInt("MultiScattering",1);
    aerial_lut_shader->setInt("ShadowMap",2);

    aerial_lut = gl->CreateTexture(GL_TEXTURE_3D);
    GL_EXPR(glTextureStorage3D(aerial_lut,1,GL_RGBA32F,aerial_lut_size.x,aerial_lut_size.y,aerial_lut_size.z));

    aerial_buffer = gl->CreateBuffer();
    GL_EXPR(glNamedBufferData(aerial_buffer,sizeof(AerialParams),nullptr,GL_STATIC_DRAW));
}
void ASRApplication::setupSkyView()
{
    if(!sky_shader){
        sky_shader = std::make_unique<Shader>("quad.vert","sky.frag");
    }
    sky_shader->use();
    sky_shader->setInt("SkyView",0);
    sky_view_buffer = gl->CreateBuffer();
    GL_EXPR(glNamedBufferData(sky_view_buffer,sizeof(SkyViewParams),nullptr,GL_STATIC_DRAW));
}
inline float deg2rad(float deg){
    return deg / 180.f * pi<float>();
}
void ASRApplication::generateShadowMap(const mat4& vp)
{
    shadow_shader->use();
    shadow_shader->setMat4("Model",terrain.local_to_world);
    shadow_shader->setMat4("NormalModel", transpose(inverse(terrain.local_to_world)));
    shadow_shader->setMat4("VP",vp);
    GL_EXPR(glBindFramebuffer(GL_FRAMEBUFFER,shadow.fbo));
    GL_EXPR(glDrawBuffer(GL_COLOR_ATTACHMENT0));
    GL_EXPR(glBindVertexArray(terrain.vao));
    GL_EXPR(glDrawElements(GL_TRIANGLES,terrain.index_count,GL_UNSIGNED_INT,nullptr));
    GL_EXPR(glBindFramebuffer(GL_FRAMEBUFFER,0));
}
void ASRApplication::generateSkyLUT(const vec3f& sun_dir,const vec3f& sun_radiance)
{
    //update sky params
    {
        sky_params.view_pos = camera->getCameraPos() * world_scale;
        sky_params.sun_dir = sun_dir;
        sky_params.sun_intensity = sun_radiance;
        GL_EXPR(glNamedBufferSubData(sky_buffer,0,sizeof(SkyParams),&sky_params));
    }
    GL_EXPR(glBindBufferBase(GL_UNIFORM_BUFFER,0,atmosphere_buffer));
    GL_EXPR(glBindBufferBase(GL_UNIFORM_BUFFER,1,sky_buffer));
    GL_EXPR(glBindTextureUnit(0,transmittance_lut));
    GL_EXPR(glBindTextureUnit(1,multiscattering_lut));
    GL_EXPR(glBindSampler(0,lut_sampler));
    GL_EXPR(glBindSampler(1,lut_sampler));
    sky_lut_shader->use();
    GL_EXPR(glBindFramebuffer(GL_FRAMEBUFFER,sky_lut_frame.fbo));
    GL_EXPR(glViewport(0,0,sky_lut_size.x,sky_lut_size.y));
    GL_EXPR(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    GL_EXPR(glDrawBuffer(GL_COLOR_ATTACHMENT0));
    GL_EXPR(glDrawArrays(GL_TRIANGLE_STRIP,0,4));
    GL_EXPR(glBindFramebuffer(GL_FRAMEBUFFER,0));
    GL_EXPR(glViewport(0,0,window_w,window_h));
}
void ASRApplication::generateAerialLUT(const vec3f &sun_dir, const mat4 &sun_vp)
{
    //update aerial params
    {
        //todo ...
        GL_EXPR(glNamedBufferSubData(aerial_buffer,0,sizeof(AerialParams),&aerial_params));
    }
    GL_EXPR(glBindBufferBase(GL_UNIFORM_BUFFER,0,atmosphere_buffer));
    GL_EXPR(glBindBufferBase(GL_UNIFORM_BUFFER,1,aerial_buffer));
    GL_EXPR(glBindTextureUnit(0,transmittance_lut));
    GL_EXPR(glBindTextureUnit(1,multiscattering_lut));
    GL_EXPR(glBindTextureUnit(2,shadow.shadow_tex));
    GL_EXPR(glBindImageTexture(0,aerial_lut,0,GL_FALSE,0,GL_READ_WRITE,GL_RGBA32F));
    GL_EXPR(glBindSampler(0,lut_sampler));
    GL_EXPR(glBindSampler(1,lut_sampler));
    GL_EXPR(glBindSampler(2,shadow.sampler));
    aerial_lut_shader->use();
    constexpr int group_thread_size_x = 16;
    constexpr int group_thread_size_y = 16;
    const int group_size_x = (aerial_lut_size.x + group_thread_size_x - 1) / group_thread_size_x;
    const int group_size_y = (aerial_lut_size.y + group_thread_size_y - 1) / group_thread_size_y;
    GL_EXPR(glDispatchCompute(group_size_x,group_size_y,1));
    GL_EXPR(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));
}
void ASRApplication::updateFrustumDirs(const mat4& inv_view_proj)
{
    const vec4f A0 = inv_view_proj * vec4f(-1, 1, 0.2f, 1);
    const vec4f A1 = inv_view_proj *vec4f(-1, 1, 0.5f, 1);

    const vec4f B0 = inv_view_proj * vec4f(1, 1, 0.2f, 1);
    const vec4f B1 = inv_view_proj * vec4f(1, 1, 0.5f, 1);

    const vec4f C0 = inv_view_proj * vec4f(-1, -1, 0.2f, 1);
    const vec4f C1 = inv_view_proj * vec4f(-1, -1, 0.5f, 1);

    const vec4f D0 = inv_view_proj * vec4f(1, -1, 0.2f, 1);
    const vec4f D1 = inv_view_proj * vec4f(1, -1, 0.5f, 1);

    frustum_dirs.frustum_a = normalize(vec3f(A1 / A1.w - A0 / A0.w));
    frustum_dirs.frustum_b = normalize(vec3f(B1 / B1.w - B0 / B0.w));
    frustum_dirs.frustum_c = normalize(vec3f(C1 / C1.w - C0 / C0.w));
    frustum_dirs.frustum_d = normalize(vec3f(D1 / D1.w - D0 / D0.w));
}
void ASRApplication::renderMeshes(const vec3f &sun_dir, const vec3f &sun_radiance, const mat4 &sun_vp)
{

}
void ASRApplication::renderSky()
{
    //update sky view params
    {
        sky_view_params.frustum_a = frustum_dirs.frustum_a;
        sky_view_params.frustum_b = frustum_dirs.frustum_b;
        sky_view_params.frustum_c = frustum_dirs.frustum_c;
        sky_view_params.frustum_d = frustum_dirs.frustum_d;
        GL_EXPR(glNamedBufferSubData(sky_view_buffer,0,sizeof(SkyViewParams),&sky_view_params));
    }
    GL_EXPR(glBindBufferBase(GL_UNIFORM_BUFFER,0,sky_view_buffer));
    GL_EXPR(glBindTextureUnit(0,sky_lut));
    GL_EXPR(glBindSampler(0,lut_sampler));
    sky_shader->use();
    GL_EXPR(glDrawArrays(GL_TRIANGLE_STRIP,0,4));
}
void ASRApplication::renderSunDisk(const vec3f &sun_dir, const vec3f &sun_radiance)
{

}
void ASRApplication::render_frame()
{
    GL_EXPR(glClearColor(0,0,0,0));
    GL_EXPR(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));
    float sun_y_rad = deg2rad(-sun_y_degree);
    float sun_x_rad = deg2rad(sun_x_degree);
    float sun_dir_y = std::sin(sun_y_rad);
    float sun_dir_x = std::cos(sun_y_rad) * std::cos(sun_x_rad);
    float sun_dir_z = std::cos(sun_y_rad) * std::sin(sun_x_rad);
    vec3 sun_dir = {sun_dir_x,sun_dir_y,sun_dir_z};
    vec3 sun_radiance = sun_color * sun_intensity;
    auto view = lookAt(-sun_dir * 20.f,{0.f,0.f,0.f},{0.f,1.f,0.f});
    auto proj = ortho(-10.f,10.f,-10.f,10.f,0.1f,80.f);
    auto vp = proj * view;

    auto camera_view = camera->getViewMatrix();
    auto camera_proj = perspective(deg2rad(camera->getZoom()),window_w * 1.f / window_h,0.1f,30.f);

    updateFrustumDirs(inverse(camera_proj * camera_view));

    generateShadowMap(vp);

    generateSkyLUT(sun_dir,sun_radiance);

    generateAerialLUT(sun_dir,vp);

    GL_EXPR(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));

    if(enable_terrain){
        renderMeshes(sun_dir,sun_radiance,vp);
    }

    if(enable_sky){
        renderSky();
    }

    if(enable_sun_disk){
        renderSunDisk(sun_dir,sun_radiance);
    }

}
void ASRApplication::render_imgui()
{
    ImGui::Begin("Atmosphere Settings");
    ImGui::Text("fps: %f",ImGui::GetIO().Framerate);
    if(ImGui::TreeNode("Sun")){
        ImGui::SliderFloat("Sun Angle X (Degree)",&sun_x_degree,0,360);
        ImGui::SliderFloat("Sun Angle Y (Degree)",&sun_y_degree,-10,90);
        ImGui::SliderFloat("Sun Intensity",&sun_intensity,0,20);

        ImGui::TreePop();
    }

    ImGui::End();
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