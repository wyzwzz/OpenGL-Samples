#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace gl
{
using vec4f = glm::vec4;
using vec3f = glm::vec3;
using vec2f = glm::vec2;
using vec2i = glm::vec<2,int>;
using vec3i = glm::vec<3,int>;
struct vertex_t
{
    vec3f pos;
    vec3f normal;
    vec2f tex_coord;
    bool operator==(const vertex_t &other) const
    {
        return pos == other.pos && normal == other.normal && tex_coord == other.tex_coord;
    }
};

struct triangle_t
{
    vertex_t vertices[3];
};

struct mesh_t
{
    std::string name;
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> tex_coords;
    std::vector<uint32_t> indices;
};

struct Mesh
{
    std::string name;
    std::vector<vertex_t> vertices;
    std::vector<uint32_t> indices;

};

Mesh load_mesh_from_gltf(const std::string& filename);

Mesh load_mesh_from_obj(const std::string &filename);

std::vector<mesh_t> load_mesh_t_from_obj(const std::string &filename);

std::vector<triangle_t> load_triangles_from_obj(const std::string &filename);

} // namespace gl