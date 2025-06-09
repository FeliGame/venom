#ifndef VK_MESH_H
#define VK_MESH_H

/// 本文件负责掌管点Vertex - 格Mesh - 体RenderObject

#include <vk_types.h>
#include <vector>
#include <glm/glm.hpp>                  // 线性代数（向量、矩阵）
#include <glm/gtc/matrix_transform.hpp> // MVP获取
#include <glm/vec2.hpp>                 // now needed for the Vertex struct
#include <tiny_obj_loader.h>
#include <iostream>

// Vulkan输入渲染管线的顶点数据信息
struct VertexInputDescription
{
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Vertex
{
    glm::vec3 position; // Location 0
    glm::vec3 normal;   // Location 1
    glm::vec3 color;    // Location 2
    glm::vec2 uv;       // Location 3，图片像素的水平/垂直坐标

    static VertexInputDescription get_vertex_description();
};

struct Mesh
{
    std::vector<Vertex> _vertices;
    AllocatedBuffer _vertexBuffer;

    bool load_from_obj(const char *filename);
};

static const float LIGHT_SCALE = 1 / 16.f; // 光强刻度
struct RenderObject
{
    Mesh *mesh;
    Material *material;
    glm::mat4 model_transform;
    glm::vec4 objectLight{0.f, 0.f, 0.f, LIGHT_SCALE}; //  物体自身光强，xyz为自身光照色彩，w所在格亮度（1.0满亮度）
    glm::vec3 normal;
};

#endif /* VK_MESH_H */
