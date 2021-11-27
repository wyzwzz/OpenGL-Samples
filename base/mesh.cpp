//
// Created by wyz on 2021/11/10.
//
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include "mesh.hpp"
#include "logger.hpp"
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
