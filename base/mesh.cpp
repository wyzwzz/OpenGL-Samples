//
// Created by wyz on 2021/11/10.
//
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include "mesh.hpp"
#include "logger.hpp"
#include <unordered_map>
#include <glm/gtx/hash.hpp>
std::vector<mesh_t> load_meshed_from_obj(const std::string &filename)
{
    tinyobj::ObjReader reader;
    if(!reader.ParseFromFile(filename)){
        throw std::runtime_error(reader.Error());
    }
    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();

    std::vector<mesh_t> meshes;

    for(auto& shape:shapes){
        for(auto face_vertex_count : shape.mesh.num_face_vertices)
        {
            if(face_vertex_count != 3)
            {
                throw std::runtime_error(
                    "invalid obj face vertex count: " +
                    std::to_string(+face_vertex_count));
            }
        }
        if(shape.mesh.indices.size() % 3 != 0)
        {
            throw std::runtime_error(
                "invalid obj index count: " +
                std::to_string(shape.mesh.indices.size()));
        }

        const size_t triangle_count = shape.mesh.indices.size() / 3;
        const size_t vertex_count = attrib.vertices.size() / 3;
        LOG_INFO("triangle count {0}",triangle_count);
        LOG_INFO("vertex count {0}",vertex_count);
        meshes.emplace_back();
        auto& mesh = meshes.back();
        mesh.positions.resize(vertex_count*3);
        mesh.normals.resize(vertex_count*3);
        mesh.tex_coords.resize(vertex_count*2);
        for(size_t i = 0;i<triangle_count;i++){
            for(int k =0;k<3;k++){
                auto index = shape.mesh.indices[i*3+k];
                mesh.indices.push_back(index.vertex_index);
                for(int t = 0;t < 3; t++){
                    mesh.positions[index.vertex_index * 3 + t] = attrib.vertices[3 * index.vertex_index + t];
                    mesh.normals[index.vertex_index * 3 + t] = attrib.normals[3 * index.normal_index + t];
                }
                for(int t=0;t<2;t++){
                    mesh.tex_coords[index.vertex_index*2+t] = attrib.texcoords[2*index.texcoord_index+t];
                }
            }
        }
    }
    return meshes;
}

namespace std{
template <> struct hash<vertex_t>{
    size_t operator()(const vertex_t& vertex) const{
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.tex_coord) << 1);
    }
};
}

Mesh load_mesh_from_obj(const std::string& filename){
        tinyobj::ObjReader reader;
    if(!reader.ParseFromFile(filename)){
        throw std::runtime_error(reader.Error());
    }
    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();

    Mesh mesh;


    std::unordered_map<vertex_t,uint32_t> unique_vertices{};

    for(const auto& shape:shapes){
        for(const auto& index:shape.mesh.indices){
            vertex_t vertex{};
            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]
            };
            vertex.tex_coord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                attrib.texcoords[2 * index.texcoord_index + 1]
            };
            if(unique_vertices.count(vertex) == 0){
                unique_vertices[vertex] = static_cast<uint32_t>(mesh.vertices.size());
                mesh.vertices.push_back(vertex);
            }
            mesh.indices.push_back(unique_vertices[vertex]);
        }
    }
    return mesh;
}

std::vector<triangle_t> load_triangles_from_obj(const std::string& filename){
    tinyobj::ObjReader reader;
    if(!reader.ParseFromFile(filename)){
        throw std::runtime_error(reader.Error());
    }
    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();

    std::vector<triangle_t> triangles;

    for(auto& shape:shapes){
        for(auto face_vertex_count : shape.mesh.num_face_vertices)
        {
            if(face_vertex_count != 3)
            {
                throw std::runtime_error(
                    "invalid obj face vertex count: " +
                    std::to_string(+face_vertex_count));
            }
        }
        if(shape.mesh.indices.size() % 3 != 0)
        {
            throw std::runtime_error(
                "invalid obj index count: " +
                std::to_string(shape.mesh.indices.size()));
        }
        const size_t triangle_count = shape.mesh.indices.size() / 3;
        const size_t vertex_count = attrib.vertices.size() / 3;
        LOG_INFO("triangle count {0}",triangle_count);
        LOG_INFO("vertex count {0}",vertex_count);
        for(size_t i = 0;i < triangle_count; i++){
            triangle_t tri;
            for(int k=0;k<3;k++){
                auto index = shape.mesh.indices[i*3+k];
                tri.vertices[k].pos = {attrib.vertices[3*index.vertex_index + 0],
                attrib.vertices[3*index.vertex_index+1],attrib.vertices[3*index.vertex_index+2]};
                
                tri.vertices[k].normal={attrib.normals[3*index.normal_index+0],
                attrib.vertices[3*index.normal_index+1],3*index.normal_index+2};
                
                tri.vertices[k].tex_coord={attrib.texcoords[2*index.texcoord_index+0],
                attrib.texcoords[2*index.texcoord_index+1]};
            }
            triangles.emplace_back(tri);
        }
    }
    return triangles;
}