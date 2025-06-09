#ifndef vk_types_h
#define vk_types_h
#include "vulkan/vulkan.h"
#include <vma/vk_mem_alloc.h>
#include <glm/glm.hpp> //线性代数（向量、矩阵）

// 分配好的缓存
struct AllocatedBuffer
{
    VkBuffer _buffer;
    VmaAllocation _allocation; // 分配信息，如大小，分配的内存来源
};

// 分配好的图片
struct AllocatedImage
{
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

// FrameData和UploadContext是对等的数据结构，只是作用不同
// 帧同步数据结构（若干个该数据结构元素组成多重缓冲）
struct FrameData
{
    VkSemaphore _presentSemaphore, _renderSemaphore;
    VkFence _renderFence;

    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;

    AllocatedBuffer cameraBuffer;
    VkDescriptorSet globalDescriptorSet;

    AllocatedBuffer objectBuffer;
    VkDescriptorSet objectDescriptorSet;
};

// 内存到显存同步数据的专用数据结构
struct UploadContext
{
    VkFence _uploadFence; // 多个upload_mesh调用，要保证先后执行
    VkCommandPool _commandPool;
    VkCommandBuffer _commandBuffer;
};

// 本质上就是AllocatedImage，但是用AllocatedImage在stbi分配时会报错
struct Texture
{
    AllocatedImage image;
    VkImageView imageView;
};

// vk_engine中descriptors使用，和shader中的定义严格对应
struct GPUCameraData
{
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 proj = glm::mat4(1.0f);
    glm::vec4 camPos;
};

// 传递到片元着色器，GPU传入数据建议只包含vec4和mat4类型，并且需要手动对齐到GPU支持
struct GPUSceneData
{
    glm::vec4 fogColor{0.5f, 1.f, 1.f, 0.f};    // w is for exponent
    glm::vec4 fogDistance{16.f, 0.f, 2.f, 0.f}; // x for min, y for max, z 越大雾气扩大速度越慢, w unused.
    glm::vec4 ambientColor;
    glm::vec4 sunPos; // w for sun power
    glm::vec4 sunlightColor;
};

struct GPUObjectData
{
    glm::mat4 modelMatrix;
    glm::vec4 objectLight; // 物体自身光强，xyz为自身光照色彩，w所在格亮度（1.0满亮度）
    glm::vec4 normal;      // 面的法向量
};

// note that we store the VkPipeline and layout by value, not pointer.
// They are 64 bit handles to internal driver structures anyway so storing pointers to them isn't very useful
struct Material
{
    VkDescriptorSet textureSet{VK_NULL_HANDLE}; // texture defaulted to null
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
};

#endif /* vk_types_h */
