#ifndef VK_PIPELINE_BUILDER_H
#define VK_PIPELINE_BUILDER_H

#include <vulkan/vulkan_core.h>
#include <vector>
#include <iostream>

// 图形渲染管线初始化很繁重，所以使用封装（计算渲染管线相对简单很多）
class PipelineBuilder
{
public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
    VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkViewport _viewport;
    VkRect2D _scissor; // 渲染区域
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineLayout _pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;

    VkPipeline build(VkDevice device, VkRenderPass pass);
};

#endif  // VK_PIPELINE_BUILDER_H