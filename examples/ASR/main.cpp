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

    void setupMesh();

    void setupSkyView();

    void setupSunDisk();

    void generateShadowMap(const mat4& vp);

    void generateSkyLUT(const vec3f& sun_dir,const vec3f& sun_radiance);

    void generateAerialLUT(const vec3f& sun_dir,const mat4& sun_vp);

    void updateFrustumDirs(const mat4& inv_view_proj);

    void renderMeshes(const vec3f& sun_dir,const vec3f& sun_radiance,const mat4& sun_vp,const mat4& camera_vp);

    void renderSky();

    void renderSunDisk(const vec3f& sun_dir,const vec3f& sun_radiance,const mat4& camera_vp);
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

    float max_aerial_distance = 3000;

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

    struct MeshParams{
        vec3 sun_dir;
        float sun_theta;
        vec3 sun_intensity;
        float max_aeraial_distance;
        vec3 view_pos;
        float world_scale;
        mat4 shadow_vp;
    }mesh_params;
    std::unique_ptr<Shader> mesh_shader;
    GL::GLBuffer mesh_buffer;
    GL::GLTexture mesh_albedo_tex;
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
        int slice_z_count = 32;
        float padding[2];
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

    int sun_disk_segment_count = 64;
    std::unique_ptr<Shader> sun_shader;
    struct{
        GL::GLVertexArray vao;
        GL::GLBuffer vbo;
        GL::GLBuffer ebo;
        int index_count;
        int seg_count ;
        float radius = 0.04649f;
    }sun;

    std::unique_ptr<Shader> postprocess_shader;
};
void ASRApplication::initResource()
{
    gl->SetVSync(0);

    GL_EXPR(glEnable(GL_DEPTH_TEST));

    camera = std::make_unique<control::FPSCamera>(glm::vec3{4.087f,3.7f,3.957f});
    camera->setYawPitchDeg(180,0);
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
    setupMesh();
    //
    setupSkyView();

    setupSunDisk();
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
void ASRApplication::setupMesh()
{
    if(!mesh_shader){
        mesh_shader = std::make_unique<Shader>("mesh.vert","mesh.frag");
    }
    mesh_shader->use();
    mesh_shader->setInt("Transmittance",0);
    mesh_shader->setInt("AerialPerspective",1);
    mesh_shader->setInt("ShadowMap",2);
    mesh_shader->setInt("AlbedoMap",3);
    mesh_buffer = gl->CreateBuffer();
    GL_EXPR(glNamedBufferData(mesh_buffer,sizeof(MeshParams),nullptr,GL_STATIC_DRAW));

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
std::vector<vec2f> createDiskVertices(int seg_count){
    std::vector<vec2f> vertices;
    constexpr float PI = pi<float>();
    for(int i = 0; i < seg_count; ++i){
        float phi0 = float(i) / seg_count * 2 * PI;
        float phi1 = float(i + 1) / seg_count * 2 * PI;
        vertices.emplace_back(0);
        vertices.emplace_back(std::cos(phi0),std::sin(phi0));
        vertices.emplace_back(std::cos(phi1),std::sin(phi1));
    }
    return vertices;
}
void ASRApplication::setupSunDisk()
{
    if(!sun_shader){
        sun_shader = std::make_unique<Shader>("sun.vert","sun.frag");
    }
    sun_shader->use();
    sun_shader->setInt("Transmittance",0);
    {
        CreateSphere(gl,sun.vao,sun.vbo,sun.ebo,sun.index_count);
    }
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
    GL_EXPR(glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT));
    GL_EXPR(glBindVertexArray(terrain.vao));
    GL_EXPR(glViewport(0,0,ShadowMapSize.x,ShadowMapSize.y));
    GL_EXPR(glDrawElements(GL_TRIANGLES,terrain.index_count,GL_UNSIGNED_INT,nullptr));
    GL_EXPR(glViewport(0,0,window_w,window_h));
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
    GL_EXPR(glDisable(GL_DEPTH_TEST));
    GL_EXPR(glDrawArrays(GL_TRIANGLE_STRIP,0,4));
    GL_EXPR(glEnable(GL_DEPTH_TEST));
    GL_EXPR(glBindFramebuffer(GL_FRAMEBUFFER,0));
    GL_EXPR(glViewport(0,0,window_w,window_h));
}
void ASRApplication::generateAerialLUT(const vec3f &sun_dir, const mat4 &sun_vp)
{
    //update aerial params
    {
        aerial_params.sun_dir = sun_dir;
        aerial_params.sun_theta = std::asin(-sun_dir.y);
        aerial_params.frustum_a = frustum_dirs.frustum_a;
        aerial_params.max_view_distance = max_aerial_distance;
        aerial_params.frustum_b = frustum_dirs.frustum_b;
        aerial_params.ray_march_step_count_per_slice = 4;
        aerial_params.frustum_c = frustum_dirs.frustum_c;
        aerial_params.enable_multiscattering = enalbe_multiscattering;
        aerial_params.frustum_d = frustum_dirs.frustum_d;
        aerial_params.view_height = camera->getCameraPos().y * world_scale;
        aerial_params.view_pos = camera->getCameraPos();
        aerial_params.enable_shadow = enable_shadow;
        aerial_params.shadow_vp = sun_vp;
        aerial_params.world_scale = world_scale;
        aerial_params.slice_z_count = aerial_lut_size.z;
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
void ASRApplication::renderMeshes(const vec3f &sun_dir, const vec3f &sun_radiance, const mat4 &sun_vp,const mat4& camera_vp)
{
    //update mesh params
    {
        mesh_params.sun_dir = sun_dir;
        mesh_params.sun_theta = std::asin(-sun_dir.y);
        mesh_params.sun_intensity = sun_radiance;
        mesh_params.max_aeraial_distance = max_aerial_distance;
        mesh_params.view_pos = camera->getCameraPos();
        mesh_params.world_scale = world_scale;
        mesh_params.shadow_vp = sun_vp;
        GL_EXPR(glNamedBufferSubData(mesh_buffer,0,sizeof(MeshParams),&mesh_params));
    }
    GL_EXPR(glBindBufferBase(GL_UNIFORM_BUFFER,0,atmosphere_buffer));
    GL_EXPR(glBindBufferBase(GL_UNIFORM_BUFFER,1,mesh_buffer));
    GL_EXPR(glBindTextureUnit(0,transmittance_lut));
    GL_EXPR(glBindTextureUnit(1,aerial_lut));
    GL_EXPR(glBindTextureUnit(2,shadow.shadow_tex));
    GL_EXPR(glBindTextureUnit(3,mesh_albedo_tex));
    GL_EXPR(glBindSampler(0,lut_sampler));
    GL_EXPR(glBindSampler(1,lut_sampler));
    GL_EXPR(glBindSampler(2,lut_sampler));
    GL_EXPR(glBindSampler(3,lut_sampler));
    mesh_shader->use();
    mesh_shader->setMat4("Model",terrain.local_to_world);
    mesh_shader->setMat4("NormalModel", transpose(inverse(terrain.local_to_world)));
    mesh_shader->setMat4("VP",camera_vp);
    GL_EXPR(glDrawElements(GL_TRIANGLES,terrain.index_count,GL_UNSIGNED_INT,nullptr));
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
    GL_EXPR(glDepthFunc(GL_LEQUAL));
    GL_EXPR(glDrawArrays(GL_TRIANGLE_STRIP,0,4));
    GL_EXPR(glDepthFunc(GL_LESS));
}
void ASRApplication::renderSunDisk(const vec3f &sun_dir, const vec3f &sun_radiance,const mat4& camera_vp)
{
    mat4 model = mat4(1);
    {
        model = scale(mat4(1.f),vec3f(sun.radius)) * model;
        model = translate(mat4(1.f),-sun_dir * 10.f) * translate(mat4(1.f),camera->getCameraPos())* model;
    }
    GL_EXPR(glBindTextureUnit(0,transmittance_lut));
    GL_EXPR(glBindBufferBase(GL_UNIFORM_BUFFER,0,atmosphere_buffer));
    sun_shader->use();
    sun_shader->setMat4("MVP",camera_vp * model);
    sun_shader->setFloat("sun_theta",std::asin(-sun_dir.y));
    sun_shader->setFloat("view_height",camera->getCameraPos().y * world_scale);
    sun_shader->setVec3("SunRadiance",sun_radiance);
    GL_EXPR(glBindVertexArray(sun.vao));
    GL_EXPR(glDepthFunc(GL_LEQUAL));
    GL_EXPR(glDrawElements(GL_TRIANGLE_STRIP,sun.index_count,GL_UNSIGNED_INT,nullptr));
    GL_EXPR(glDepthFunc(GL_LESS));
    GL_EXPR(glBindVertexArray(0));
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

//    GL_EXPR(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));

    if(enable_terrain){
        renderMeshes(sun_dir,sun_radiance,vp,camera_proj * camera_view);
    }

    if(enable_sky){
        renderSky();
    }

    if(enable_sun_disk){
        renderSunDisk(sun_dir,sun_radiance,camera_proj * camera_view);
    }

}
void ASRApplication::render_imgui()
{
    ImGui::Begin("Atmosphere Settings");

    ImGui::Checkbox("Enable Multi Scattering", &enalbe_multiscattering);
    ImGui::Checkbox("Enable Sky",              &enable_sky);
    ImGui::Checkbox("Enable Shadow",           &enable_shadow);
    ImGui::Checkbox("Enable Sun Disk",         &enable_sun_disk);
    ImGui::Checkbox("Enable Terrain",          &enable_terrain);

    ImGui::Text("fps: %f",ImGui::GetIO().Framerate);
    if(ImGui::TreeNode("Sun")){
        ImGui::SliderFloat("Sun Angle X (Degree)",&sun_x_degree,0,360);
        ImGui::SliderFloat("Sun Angle Y (Degree)",&sun_y_degree,-10,90);
        ImGui::SliderFloat("Sun Intensity",&sun_intensity,0,20);

        ImGui::TreePop();
    }
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::TreeNode("Sky LUT"))
    {

        ImGui::TreePop();
    }
    ImGui::End();
}
void ASRApplication::process_input()
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