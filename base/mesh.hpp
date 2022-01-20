//
// Created by wyz on 2021/11/10.
//
#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <string>
using vec3f = glm::vec3;
using vec2f = glm::vec2;

struct vertex_t{
    vec3f pos;
    vec3f normal;
    vec2f tex_coord;
    bool operator==(const vertex_t& other) const{
        return pos == other.pos && normal == other.normal && tex_coord == other.tex_coord;
    }
};

struct triangle_t{
    vertex_t vertices[3];
};

struct mesh_t{
    std::string name;
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> tex_coords;
    std::vector<uint32_t> indices;
};

struct Mesh{
    std::vector<vertex_t> vertices;
    std::vector<uint32_t> indices;
};

Mesh load_mesh_from_obj(const std::string& filename);

std::vector<mesh_t> load_meshed_from_obj(const std::string& filename);

std::vector<triangle_t> load_triangles_from_obj(const std::string& filename);