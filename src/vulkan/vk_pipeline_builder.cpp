#include "vk_pipeline_builder.h"

// 图形渲染管线初始化很繁重，所以使用封装（计算渲染管线相对简单很多）
VkPipeline PipelineBuilder::build(VkDevice device, VkRenderPass pass)
{
    // 1.用于支持多视口、多渲染区域
    // make viewport state from our stored viewport and scissor.
    // at the moment we won't support multi          ple viewports or scissors
    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .viewportCount = 1,
        .pViewports = &_viewport,
        .scissorCount = 1,
        .pScissors = &_scissor};

    // 2.配置ColorBlend：需要和片元着色器的输出搭配
    // setup dummy color blending. We aren't using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &_colorBlendAttachment};

    // 3.创建渲染管线
    // we now use all of the info structs we have been writing into into this one to create the pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .stageCount = static_cast<uint32_t>(_shaderStages.size()),
        .pStages = _shaderStages.data(),
        .pVertexInputState = &_vertexInputInfo,
        .pInputAssemblyState = &_inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &_rasterizer,
        .pMultisampleState = &_multisampling,
        .pDepthStencilState = &_depthStencil,
        .pColorBlendState = &colorBlending,
        .layout = _pipelineLayout,
        .renderPass = pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE};

    // it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(
            device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS)
    {
        std::cout << "failed to create pipeline\n";
        return VK_NULL_HANDLE; // failed to create graphics pipeline
    }
    else
    {
        return newPipeline;
    }
}
