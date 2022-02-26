//
// Created by wyz on 2021/11/11.
//
#include "../common.hpp"
#include <demo.hpp>
#include <shader_program.hpp>
class ShadowMapApplication final : public Demo
{
    void initResource() override
    {
        glEnable(GL_DEPTH_TEST);

        const std::string marry_model_path = ModelPath + "mary/Marry.obj";
        const std::string floor_model_path = ModelPath + "floor/floor.obj";

        meshes = load_mesh_t_from_obj(marry_model_path);

        auto floor = load_mesh_t_from_obj(floor_model_path);
        meshes.insert(meshes.end(), floor.begin(), floor.end());

        for (auto &mesh : meshes)
        {
            if (mesh.positions.size() != mesh.normals.size() || mesh.positions.size() / 3 * 2 != mesh.tex_coords.size())
            {
                throw std::runtime_error("mesh error");
            }
            uint32_t vao;
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            uint32_t vbo;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, mesh.positions.size() * sizeof(float) / 3 * 8, nullptr, GL_STATIC_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, mesh.positions.size() * sizeof(float), mesh.positions.data());
            glBufferSubData(GL_ARRAY_BUFFER, mesh.positions.size() * sizeof(float), mesh.normals.size() * sizeof(float),
                            mesh.normals.data());
            glBufferSubData(GL_ARRAY_BUFFER, mesh.positions.size() * sizeof(float) * 2,
                            mesh.tex_coords.size() * sizeof(float), mesh.tex_coords.data());

            uint32_t ebo;
            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(uint32_t), mesh.indices.data(),
                         GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                                  (void *)(mesh.positions.size() * sizeof(float)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                                  (void *)(mesh.positions.size() * sizeof(float) * 2));
            glEnableVertexAttribArray(2);

            glBindVertexArray(0);
            GL_CHECK
            mesh_vao.push_back(vao);
            mesh_vbo.push_back(vbo);
            mesh_ebo.push_back(ebo);
        }

        // init shadow fbo
        {
            shadow_fbo = gl->CreateFramebuffer();
            glBindFramebuffer(GL_FRAMEBUFFER, shadow_fbo);
            shadow_rbo = gl->CreateRenderbuffer();
            glBindRenderbuffer(GL_RENDERBUFFER, shadow_rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, shadow_w, shadow_h);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
            GL_CHECK
            shadow_tex = gl->CreateTexture(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, shadow_tex);
            // texture binding 0
            glBindTextureUnit(0, shadow_tex);
            GL_CHECK
            glTextureStorage2D(shadow_tex, 1, GL_RGBA32F, shadow_w, shadow_h);
            GL_CHECK
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            GL_CHECK
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadow_tex, 0);
            GL_CHECK
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                std::cout << std::hex << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
                throw std::runtime_error("Framebuffer object is not complete!");
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            GL_CHECK
        }
        shadow_shader = std::make_unique<Shader>((SourcePath + "ShadowMap/shader/shadow_v.glsl").c_str(),
                                                 (SourcePath + "ShadowMap/shader/shadow_f.glsl").c_str());
        phong_shader = std::make_unique<Shader>((SourcePath + "ShadowMap/shader/phong_v.glsl").c_str(),
                                                (SourcePath + "Shadowmap/shader/phong_f.glsl").c_str());

        light.position = glm::vec3(5.f, 5.f, 0.f);
        light.direction = glm::normalize(glm::vec3{-2.f, -2.f, 0.f});

        camera = std::make_unique<control::FPSCamera>(glm::vec3{0.f, 1.5f, 5.f});
        //        camera = std::make_unique<control::FPSCamera>(light.position);
    }
    void render_frame() override;

  public:
    ShadowMapApplication() = default;

  private:
    struct Light
    {
        vec3f position;
        vec3f direction;
        vec3f incidence; // color rgb
    } light;

    std::vector<mesh_t> meshes;

    std::unique_ptr<Shader> shadow_shader;
    std::unique_ptr<Shader> phong_shader;

    std::vector<uint32_t> mesh_vao, mesh_vbo, mesh_ebo;

    GL::GLFramebuffer shadow_fbo;
    GL::GLRenderbuffer shadow_rbo;
    GL::GLTexture shadow_tex;

    const int shadow_w = 4096, shadow_h = 4096;
};
void ShadowMapApplication::render_frame()
{
    glClearColor(0.f, 0.f, 0.f, 0.f);

    glBindFramebuffer(GL_FRAMEBUFFER, shadow_fbo);
    glViewport(0, 0, shadow_w, shadow_h);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    auto light_view =
        glm::lookAt(light.position, light.position + light.direction, glm::normalize(glm::vec3{-1.f, 1.f, 0.f}));
    //    auto light_projection = glm::perspective(glm::radians(60.f),float(shadow_w)/shadow_h,0.1f,10.f);
    auto light_projection = glm::ortho(-5.f, 5.f, -5.f, 5.f, 0.1f, 10.f);
    auto light_mvp = light_projection * light_view;
    shadow_shader->use();
    shadow_shader->setMat4("MVPMatrix", light_mvp);
    for (int i = 0; i < meshes.size(); i++)
    {
        GL_EXPR(glBindVertexArray(mesh_vao[i]));
        GL_EXPR(glDrawElements(GL_TRIANGLES, meshes[i].indices.size(), GL_UNSIGNED_INT, 0));
        GL_CHECK
    }
    GL_CHECK

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, window_w, window_h);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    auto camera_view = camera->getViewMatrix();
    auto camera_projection = glm::perspective(glm::radians(camera->getZoom()), float(window_w) / window_h, 0.1f, 20.f);
    auto camera_mvp = camera_projection * camera_view;
    phong_shader->use();
    phong_shader->setMat4("LightMVPMatrix", light_mvp);
    phong_shader->setMat4("CameraMVPMatrix", camera_mvp);
    phong_shader->setVec3("ligth_dir", glm::normalize(glm::vec3{-1.f, -1.f, -1.f}));
    phong_shader->setVec3("camera_pos", camera->getCameraPos());
    phong_shader->setInt("ShadowMap", 0);
    for (int i = 0; i < meshes.size(); i++)
    {
        GL_EXPR(glBindVertexArray(mesh_vao[i]));
        GL_EXPR(glDrawElements(GL_TRIANGLES, meshes[i].indices.size(), GL_UNSIGNED_INT, 0));
        GL_CHECK
    }
    GL_CHECK
}
int main()
{
    try
    {
        ShadowMapApplication app;
        app.run();
    }
    catch (const std::exception &err)
    {
        std::cout << err.what() << std::endl;
    }
    return 0;
}