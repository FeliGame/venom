#ifndef VK_INIT_H
#define VK_INIT_H

#include <vk_types.h>

// 封装了VkBootstrap没有封装的初始化操作
namespace vkinit
{
    VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);

    VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);
    VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);

    // 渲染管线着色器阶段会读取着色器文件
    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule);

    // 顶点缓冲区、顶点格式
    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();

    // 输入汇编器，决定将顶点画成三角形组、直线还是点
    // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : normal triangle drawing
    // VK_PRIMITIVE_TOPOLOGY_POINT_LIST : points
    // VK_PRIMITIVE_TOPOLOGY_LINE_LIST : line-list
    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info(VkPrimitiveTopology topology);

    // 光栅化阶段：线段宽度、背面剔除、是否绘制线框
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(VkPolygonMode polygonMode, VkCullModeFlagBits cullMode, bool depthBiasEnable, float biasConstant = 0.0f, float biasClamp = 0.0f, float biasSlope = 0.0f);

    // MSAA级别（需要RenderPass支持！）
    VkPipelineMultisampleStateCreateInfo multisampling_state_create_info();

    // 颜色混合（渲染管线最后一阶段）
    VkPipelineColorBlendAttachmentState color_blend_attachment_unable_state();
    VkPipelineColorBlendAttachmentState color_blend_attachment_enable_state(VkBlendFactor srcBlendFactor, VkBlendFactor dstBlendFactor, VkBlendOp blendOp);

    // 渲染管线的着色器输入，例如push-constants和descriptor sets
    VkPipelineLayoutCreateInfo pipeline_layout_create_info();

    VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);

    VkImageViewCreateInfo image_view_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

    // 渲染管线中深度检测、比较的方式
    VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp);

    VkDescriptorSetLayoutBinding descriptorset_layout_binding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);

    VkDescriptorSetLayoutCreateInfo descriptorset_layout_create_info(int bindingCount, VkDescriptorSetLayoutBinding *bindings);

    VkWriteDescriptorSet write_descriptor_buffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo *bufferInfo, uint32_t binding);

    VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);

    VkSubmitInfo submit_info(VkCommandBuffer *cmd);

    VkSamplerCreateInfo sampler_create_info(VkFilter filters, VkBorderColor borderColor, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

    VkWriteDescriptorSet write_descriptor_image(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo *imageInfo, uint32_t binding);

    VkFramebufferCreateInfo framebuffer_create(VkRenderPass renderPass, VkExtent2D extent);
}

#endif /* VK_INIT_H */
