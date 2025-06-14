#include "vk_engine.h"
#include <string>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <vk_pipeline_builder.h>

void VenomApp::run()
{
    initWindow();
    initVulkan();
    tik(_current_frame);
    __clock_game_start = std::chrono::high_resolution_clock::now(); // 开始计时
    mainLoop();
    __clock_game_end = std::chrono::high_resolution_clock::now();
    std::cout << "You've played for " << std::chrono::duration_cast<std::chrono::seconds>(__clock_game_end - __clock_game_start).count() << " seconds!";
    cleanup();
}

// 重置交换链尺寸（但是不重置图像内容，从而不影响性能）
void VenomApp::resize_swapchain()
{
    vkDeviceWaitIdle(_device);
    //        destroy_swapchain();
    int w, h;
    glfwGetWindowSize(__window, &w, &h);
    _window_extent.width = std::min((int)MAX_WIDTH, w);
    _window_extent.height = std::min((int)MAX_HEIGHT, h);

    init_swapchain();
    _resize_requested = false;
}

// 类似于drawcall，提交立即执行的CPU复制缓冲指令到buffer，然后提交到graphics queue（可以提交到另外的计算专用Queue执行，实现后台线程计算）
void VenomApp::immediate_submit(std::function<void(VkCommandBuffer cmd)> &&function)
{
    vkWaitForFences(_device, 1, &_upload_context._uploadFence, true, 9999999999);
    vkResetFences(_device, 1, &_upload_context._uploadFence);

    VkCommandBuffer cmd = _upload_context._commandBuffer;
    // begin the command buffer recording. We will use this command buffer exactly once before resetting, so we tell vulkan that
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    // execute the function
    function(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submit = vkinit::submit_info(&cmd);
    // submit command buffer to the queue and execute it.
    //  _uploadFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit(_graphics_queue, 1, &submit, _upload_context._uploadFence));
    // reset the command buffers inside the command pool
    vkQueueWaitIdle(_graphics_queue);
    vkResetCommandPool(_device, _upload_context._commandPool, 0);
}

// 对齐Uniform缓冲长度，使之符合GPU最小对齐长度【Sascha Willems】
size_t VenomApp::pad_uniform_buffer_size(size_t originalSize)
{
    // Calculate required alignment based on minimum device offset alignment
    size_t minUboAlignment = _gpu_properties.limits.minUniformBufferOffsetAlignment;
    size_t alignedSize = originalSize;
    if (minUboAlignment > 0)
    {
        alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }
    return alignedSize;
}

AllocatedBuffer VenomApp::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;

    bufferInfo.size = allocSize;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = memoryUsage;

    AllocatedBuffer newBuffer;

    VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo,
                             &newBuffer._buffer,
                             &newBuffer._allocation,
                             nullptr));
    return newBuffer;
}

// 当前要渲染的帧
FrameData &VenomApp::get_current_frame()
{
    return _frames[_current_frame % MAX_FRAMES_IN_FLIGHT];
}

// 【开放接口】加载纹理数据
void VenomApp::load_texture()
{
    Texture tex;
    load_from_image((std::string(TEXTURE_DIR) + "minecraft_textures_all.png").data(), tex.image);
    VkImageViewCreateInfo imageinfo = vkinit::image_view_create_info(VK_FORMAT_R8G8B8A8_SRGB, tex.image.image, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(_device, &imageinfo, nullptr, &tex.imageView);
    _main_deletion_queue.push_function([=]()
                                       { vkDestroyImageView(_device, tex.imageView, nullptr); });
    _loaded_textures["minecraft"] = tex;
}

// 【开放接口】创建网格单元并上传，该顺序不能改：直接定义或从obj文件读取网格顶点->通过顶点缓冲区上传到显存->提供给场景管理器用于绑定渲染对象
void VenomApp::load_meshes()
{
    int isMac = 0;
    // #ifdef __APPLE__
    //     isMac = 1;
    //     Mesh air_mesh; //【macOS GPU会吞掉第一格的贴图，原因未知】
    //     air_mesh._vertices.resize(6);
    //     upload_mesh(air_mesh);
    //     __meshes[0] = air_mesh;
    // #endif
    float d = 1 / (float)TEXTURE_SIZE; // 材质包中一个贴图占据的份额
    for (int vy = 0; vy < TEXTURE_SIZE; ++vy)
    {
        for (int ux = 0; ux < TEXTURE_SIZE; ++ux)
        {
            Mesh mesh;
            mesh._vertices.resize(6);
            // vertex positions，逆时针遍历顶点被认为是网格的正面
            //  以下坐标均基于自身坐标系，且单位是屏幕空间比例
            mesh._vertices[1].position = {0.f, .0f, 0.0f}; // 左下
            mesh._vertices[0].position = {0.f, 1.f, 0.0f}; // 左上
            mesh._vertices[2].position = {1.f, 1.f, 0.0f}; // 右上
            mesh._vertices[3].position = {1.f, 1.f, 0.0f}; // 右上
            mesh._vertices[5].position = {1.f, 0.f, 0.0f}; // 右下
            mesh._vertices[4].position = {0.f, 0.f, 0.0f}; // 左下

            // vertex colors，两个彩色直角三角形拼出来的正方形
            //        mesh._vertices[0].color = { 1.f, 0.f, 0.0f };
            //        mesh._vertices[1].color = { 1.f, 1.f, 1.0f };
            //        mesh._vertices[2].color = { 0.f, 0.f, 1.0f };
            //        mesh._vertices[3].color = { 0.f, 0.f, 1.0f };
            //        mesh._vertices[4].color = { 0.f, 0.f, .0f };
            //        mesh._vertices[5].color = { 0.f, 1.f, .0f };

            for (int i = 0; i < 6; ++i)
            {
                mesh._vertices[i].uv = {
                    (1 - mesh._vertices[i].position.x + ux) * d, // x方向颠倒
                    (1 - mesh._vertices[i].position.y + vy) * d  // y方向要颠倒
                };
            }
            // we don't care about the vertex normals
            upload_mesh(mesh);
            __meshes[vy * TEXTURE_SIZE + ux + isMac] = mesh;
        }
    }
}

bool VenomApp::load_from_image(const char *file, AllocatedImage &outImage)
{
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(file, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels)
    {
        std::cout << "Failed to load texture file " << file << std::endl;
        return false;
    }

    // 和upload_mesh类似，先放到CPU侧缓冲，再复制到VkImage（类似upload_mesh）
    void *pixel_ptr = pixels;
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    // the format R8G8B8A8 matches exactly with the pixels loaded from stb_image lib
    VkFormat image_format = VK_FORMAT_R8G8B8A8_SRGB;
    // allocate temporary buffer for holding texture data to upload
    AllocatedBuffer stagingBuffer = create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    // copy data to buffer
    void *data;
    vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
    memcpy(data, pixel_ptr, static_cast<size_t>(imageSize));
    vmaUnmapMemory(_allocator, stagingBuffer._allocation);
    // we no longer need the loaded data, so we can free the pixels as they are now in the staging buffer
    stbi_image_free(pixels);

    // 创建图像缓冲（类比upload_mesh的GPU侧缓冲），区别在于dimg_info的VK_IMAGE_USAGE_SAMPLED_BIT
    VkExtent3D imageExtent{
        .width = static_cast<uint32_t>(texWidth),
        .height = static_cast<uint32_t>(texHeight),
        .depth = 1};

    VkImageCreateInfo dimg_info = vkinit::image_create_info(image_format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
    AllocatedImage newImage;
    VmaAllocationCreateInfo dimg_allocinfo = {};
    dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    // allocate and create the image
    vmaCreateImage(_allocator, &dimg_info, &dimg_allocinfo, &newImage.image, &newImage.allocation, nullptr);

    // 不能直接拷贝到Image，关于Barrier（一种同步原语）参见：https://gpuopen.com/learn/vulkan-barriers-explained/
    immediate_submit([&](VkCommandBuffer cmd)
                     {
        VkImageSubresourceRange range {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        };
        // Layout切换为传输Barrier（用于复制）
        VkImageMemoryBarrier imageBarrier_toTransfer = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .image = newImage.image,
            .subresourceRange = range,
        };
        //barrier the image into the transfer-receive layout
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);
        
        // 从CPUBuffer复制到GPUImage
        VkBufferImageCopy copyRegion = {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0, // 和mipmap level有关
                .layerCount = 1
            },
            .imageExtent = imageExtent
        };
        vkCmdCopyBufferToImage(cmd, stagingBuffer._buffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
        
        // Layout切换为只读Barrier
        VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
        imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        //barrier the image into the shader readable layout
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable); });

    _main_deletion_queue.push_function([=]()
                                       {
        vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
        vmaDestroyImage(_allocator, newImage.image, newImage.allocation); });

    std::cout << "Texture loaded successfully " << file << std::endl;
    outImage = newImage;
    return true;
}

// 分配CPU_TO_GPU缓冲区，将Mesh放到该缓冲【弃用】
// 分配CPU侧和GPU侧缓冲，将Mesh放到CPU侧缓冲，最后拷贝过去【更快】
void VenomApp::upload_mesh(Mesh &mesh)
{
    const size_t bufferSize = mesh._vertices.size() * sizeof(Vertex);
    // 直接上传顶点缓冲区（低效）参数：VK_BUFFER_USAGE_VERTEX_BUFFER_BIT +  VMA_MEMORY_USAGE_CPU_TO_GPU，然后Map memory时直接拷贝到vertex buffer上
    AllocatedBuffer stagingBuffer = create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    // 将顶点数据拷贝到上面的缓冲区（建立映射->拷贝->解除映射，对于流式数据则可以长期保持映射不解除）
    void *data;
    vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
    memcpy(data, mesh._vertices.data(), mesh._vertices.size() * sizeof(Vertex));
    vmaUnmapMemory(_allocator, stagingBuffer._allocation);

    // 接收的GPU侧缓冲
    mesh._vertexBuffer = create_buffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

    _main_deletion_queue.push_function([=]()
                                       { vmaDestroyBuffer(_allocator, mesh._vertexBuffer._buffer, mesh._vertexBuffer._allocation); });

    immediate_submit([=](VkCommandBuffer cmd)
                     {
        VkBufferCopy copy;
        copy.dstOffset = 0;
        copy.srcOffset = 0;
        copy.size = bufferSize;
        vkCmdCopyBuffer(cmd, stagingBuffer._buffer, mesh._vertexBuffer._buffer, 1, &copy); });

    vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
}

void VenomApp::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // glfw窗口初始化，禁止默认初始化openGL上下文
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);    // 是否允许窗口尺寸调节，注意更改窗口尺寸需要recreate_swapchain

    __window = glfwCreateWindow(MAX_WIDTH, MAX_HEIGHT, WINDOW_TITLE, nullptr, nullptr); // 第四个参数显示器，第五个参数只与openGL有关
    glfwSetWindowUserPointer(__window, this);                                           // 为尚未实例化的窗口设置伪指针
    glfwSetFramebufferSizeCallback(__window, framebufferResizeCallback);
}

// 从磁盘加载shader文件
bool VenomApp::load_shader_module(const std::string &filePath, VkShaderModule *outShaderModule)
{
    // 二进制读文件流
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }
    // find what the size of the file is by looking up the location of the cursor
    // because the cursor is at the end, it gives the size directly in bytes
    size_t fileSize = (size_t)file.tellg();
    // spirv expects the buffer to be on uint32, so make sure to reserve an int vector big enough for the entire file
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    // put file cursor at beginning
    file.seekg(0);
    file.read((char *)buffer.data(), fileSize);
    file.close();

    // create a new shader module, using the buffer we loaded
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .codeSize = buffer.size() * sizeof(uint32_t), // codeSize has to be in bytes, so multiply the ints in the buffer by size of int to know the real size of the buffer
        .pCode = buffer.data()};

    // check that the creation goes well.
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {   
        return false;
    }
    *outShaderModule = shaderModule;
    return true;
}

// 加载着色器等渲染管线所需（可以有多条渲染管线）【创建材质】直接从着色器文件读取纹理坐标构成网格
void VenomApp::init_pipelines(const std::string &shader_vert_path, const std::string &shader_frag_path)
{
    // 加载着色器文件
    VkShaderModule vertShader;
    if (!load_shader_module(shader_vert_path, &vertShader))
    {
        std::cout << "Error when building the vertex shader module" << std::endl;
    }
    VkShaderModule fragShader;
    if (!load_shader_module(shader_frag_path, &fragShader))
    {
        std::cout << "Error when building the colored mesh shader" << std::endl;
    }

    // 1.创建管线配置，可用于控制着色器IO
    VkPipelineLayout texPipelineLayout; // 【两个管线共用一个管线布局】
    VkPipelineLayoutCreateInfo textured_pipeline_layout_info = vkinit::pipeline_layout_create_info();
    VkDescriptorSetLayout texturedSetLayouts[] = {_global_set_layout, _object_set_layout, _single_texture_set_layout};
    textured_pipeline_layout_info.setLayoutCount = sizeof(texturedSetLayouts) / sizeof(VkDescriptorSetLayout);
    textured_pipeline_layout_info.pSetLayouts = texturedSetLayouts;
    VK_CHECK(vkCreatePipelineLayout(_device, &textured_pipeline_layout_info, nullptr, &texPipelineLayout));

    // 2.创建管线
    VkPipeline texPipeline;
    // build the stage-create-info for both vertex and fragment stages. This lets the pipeline know the shader modules per stage
    PipelineBuilder pipelineBuilder;

    // vertex input controls how to read vertices from vertex buffers.
    pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
    VertexInputDescription vertexDescription; // 【两条管线共用一个顶点输入描述】不能放在大括号里面，不然指针值会被丢弃！
                                              //        if(useVertexBuffer) {
    vertexDescription = Vertex::get_vertex_description();
    // connect the pipeline builder vertex input info to the one we get from Vertex
    pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
    pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();

    pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
    pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();
    //        }

    // input assembly is the configuration for drawing triangle lists, strips, or individual points.
    // we are just going to draw triangle list
    pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // build viewport and scissor from the swapchain extents
    pipelineBuilder._viewport.x = 0.0f;
    pipelineBuilder._viewport.y = 0.0f;
    pipelineBuilder._viewport.width = (float)_window_extent.width;
    pipelineBuilder._viewport.height = (float)_window_extent.height;
    pipelineBuilder._viewport.minDepth = 0.0f;
    pipelineBuilder._viewport.maxDepth = 1.0f;
    pipelineBuilder._scissor.offset = {0, 0};
    pipelineBuilder._scissor.extent = _window_extent;
    // configure the rasterizer to draw filled triangles
    pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE, VK_CULL_MODE, false);
    // we don't use multisampling, so just run the default one
    pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();
    // a single blend attachment with no blending and writing to RGBA
    pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_enable_state(VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD);
    pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP);

    // 构建texture管线，主要是使用的片元着色器不同
    pipelineBuilder._shaderStages.clear();
    pipelineBuilder._shaderStages.push_back(
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertShader));
    pipelineBuilder._shaderStages.push_back(
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragShader)); // 区别
    pipelineBuilder._pipelineLayout = texPipelineLayout;
    texPipeline = pipelineBuilder.build(_device, _render_pass);
    __render_system.create_material(texPipeline, texPipelineLayout, _default_material_name);

    vkDestroyShaderModule(_device, vertShader, nullptr);
    vkDestroyShaderModule(_device, fragShader, nullptr);
    _main_deletion_queue.push_function([=]()
                                       {
            vkDestroyPipeline(_device, texPipeline, nullptr);
            vkDestroyPipelineLayout(_device, texPipelineLayout, nullptr); });
}

// 初始化_vk_physical_device和_vk_device
void VenomApp::init_device_allocator_queue(class vkb::Instance &vkb_inst)
{
    // 初始化_vk_physical_device和_vk_device
    vkb::PhysicalDeviceSelector selector{vkb_inst};
    vkb::PhysicalDevice physicalDeviceInfo = selector
                                                 .set_minimum_version(1, 1)
                                                 .set_surface(_surface)
                                                 .select()
                                                 .value();

    vkb::DeviceBuilder deviceBuilder{physicalDeviceInfo};
    // 启用向shader写入数据功能
    VkPhysicalDeviceShaderDrawParametersFeatures shader_draw_parameters_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES,
        .pNext = nullptr,
        .shaderDrawParameters = VK_TRUE};

    vkb::Device vkbDevice = deviceBuilder.build().value();

    // Get the VkDevice handle used in the rest of a Vulkan application
    _device = vkbDevice.device;
    _physical_device = physicalDeviceInfo.physical_device;
    _gpu_properties = vkbDevice.physical_device.properties;
    cout << "The GPU has a minimum buffer alignment of " << _gpu_properties.limits.minUniformBufferOffsetAlignment << endl;

    // 使用vma创建一个内存分配器（后续通用）
    VmaAllocatorCreateInfo allocatorInfo = {
        .physicalDevice = _physical_device,
        .device = _device,
        .instance = _instance};
    vmaCreateAllocator(&allocatorInfo, &_allocator);
    _main_deletion_queue.push_function([&]()
                                       { vmaDestroyAllocator(_allocator); });
    // 初始化队列和队列簇
    _graphics_queue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphics_queue_family = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
}

// 初始化Swapchain，需要获取窗口尺寸来转换成_window_extent
void VenomApp::init_swapchain()
{
    vkb::SwapchainBuilder swapchainBuilder{_physical_device, _device, _surface};

    vkb::Swapchain vkbSwapchain = swapchainBuilder
                                      .use_default_format_selection()
                                      .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // use vsync present mode
                                      .set_desired_extent(_window_extent.width, _window_extent.height)
                                      .build()
                                      .value();

    // store swapchain and its related images
    _swapchain = vkbSwapchain.swapchain;
    _swapchain_images = vkbSwapchain.get_images().value();
    _swapchain_image_views = vkbSwapchain.get_image_views().value();
    _swapchain_image_format = vkbSwapchain.image_format;
    _main_deletion_queue.push_function([=]()
                                       { vkDestroySwapchainKHR(_device, _swapchain, nullptr); });
}

void VenomApp::init_depth_image()
{
    _depth_image.imageFormat = VK_FORMAT_D32_SFLOAT;
    // VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT告知该Image用于深度
    VkImageCreateInfo dimg_info = vkinit::image_create_info(_depth_image.imageFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, _depth_extent);
    VmaAllocationCreateInfo dimg_allocinfo = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY, // 分配在显存上
        .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};
    vmaCreateImage(_allocator, &dimg_info, &dimg_allocinfo, &_depth_image.image, &_depth_image.allocation, nullptr);

    VkImageViewCreateInfo dview_info = vkinit::image_view_create_info(_depth_image.imageFormat, _depth_image.image, VK_IMAGE_ASPECT_DEPTH_BIT); // VK_IMAGE_ASPECT_DEPTH_BIT告知imageView用途为深度
    VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depth_image.imageView));

    _main_deletion_queue.push_function([=]()
                                       {
            vkDestroyImageView(_device, _depth_image.imageView, nullptr);
            vmaDestroyImage(_allocator, _depth_image.image, _depth_image.allocation); });
}

// 创建_command_pool和专用于CPU拷贝到GPU指令的_upload_command_pool，他们共享同一个CommandQueue
void VenomApp::init_command_pool_and_queue()
{
    // create a command pool for commands submitted to the graphics queue.
    // we also want the pool to allow for resetting of individual command buffers
    VkCommandPoolCreateInfo uploadCommandPoolInfo = vkinit::command_pool_create_info(_graphics_queue_family);
    // create pool for upload context
    VK_CHECK(vkCreateCommandPool(_device, &uploadCommandPoolInfo, nullptr, &_upload_context._commandPool));
    _main_deletion_queue.push_function([=]()
                                       { vkDestroyCommandPool(_device, _upload_context._commandPool, nullptr); });
    // allocate the default command buffer that we will use for the instant commands
    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_upload_context._commandPool, 1);

    VkCommandBuffer cmd;
    VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_upload_context._commandBuffer));

    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        // 使用_command_pool为每一帧分配一个_command_buffer
        VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));
        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);
        VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
        _main_deletion_queue.push_function([=]()
                                           { vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr); });
    }
}

// 附件引用索引=>子管道，配置附件+子管道间依赖关系+子管道=>RenderPass
void VenomApp::init_render_pass()
{
    // the renderpass will use this color attachment.
    VkAttachmentDescription color_attachment = {
        .format = _swapchain_image_format,                // the attachment will have the format needed by the swapchain
        .samples = VK_SAMPLE_COUNT_1_BIT,                 // 1 sample, we won't do MSAA
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,            // we Clear when this attachment is loaded
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,          // we keep the attachment stored when the renderpass ends
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, // we don't care about stencil
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,    // we don't know or care about the starting layout of the attachment
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR // after the renderpass ends, the image has to be on a layout ready for display
    };

    VkAttachmentReference color_attachment_ref = {
        // attachment number will index into the pAttachments array in the parent renderpass itself
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentDescription depth_attachment = {
        .flags = 0,
        .format = _depth_image.imageFormat, // 与color_attachment不同
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE, // 与color_attachment不同
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL // 与color_attachment不同
    };
    VkAttachmentReference depth_attachment_ref = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    // 配置Subpass，并将图像和深度信息绑定上去（至少配置一条subpass）
    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
        .pDepthStencilAttachment = &depth_attachment_ref // hook the depth attachment into the subpass
    };

    // 【快速渲染方案】：COLOR不等待DEPTH同步，创建管线直接使用下述color_dependency
    //        VkSubpassDependency color_dependency = {
    //            .srcSubpass = VK_SUBPASS_EXTERNAL,
    //            .dstSubpass = 0,
    //            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    //            .srcAccessMask = 0,
    //            .dstStageMask =  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    //            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    //        };

    // 建立不同subpass之间的同步关系，渲染完COLOR才能渲染DEPTH
    VkSubpassDependency color_dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};
    VkSubpassDependency depth_dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};
    VkSubpassDependency dependencies[2] = {color_dependency, depth_dependency};

    // 使用color_attachment和depth_attachment创建_render_pass
    VkAttachmentDescription attachments[2] = {color_attachment, depth_attachment};
    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = sizeof(dependencies) / sizeof(VkSubpassDependency),
        .pDependencies = dependencies};
    VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_render_pass));
    _main_deletion_queue.push_function([=]()
                                       { vkDestroyRenderPass(_device, _render_pass, nullptr); });
}

// 初始化_framebuffers，并将初始化Swapchain和Depth时的ImageViews传到FrameBuffers
void VenomApp::init_framebuffers()
{
    // create the framebuffers for the swapchain images. This will connect the render-pass to the images for rendering
    VkFramebufferCreateInfo fb_info = vkinit::framebuffer_create(_render_pass, _window_extent);

    // grab how many images we have in the swapchain
    const uint32_t swapchain_image_count = _swapchain_images.size();
    _framebuffers = std::vector<VkFramebuffer>(swapchain_image_count);

    // 创建framebuffer时传入颜色图（对应color_attachment）和深度
    for (int i = 0; i < swapchain_image_count; i++)
    {
        VkImageView attachments[2];
        attachments[0] = _swapchain_image_views[i];
        attachments[1] = _depth_image.imageView;
        fb_info.pAttachments = attachments;
        fb_info.attachmentCount = 2;
        VK_CHECK(vkCreateFramebuffer(_device, &fb_info, nullptr, &_framebuffers[i]));

        _main_deletion_queue.push_function([=]()
                                           {
                vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);
                vkDestroyImageView(_device, _swapchain_image_views[i], nullptr); });
    }
}

// 初始化同步原语
void VenomApp::init_syncs()
{
    // we want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU command (for the first frame)
    VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    // for the semaphores we don't need any flags
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));
        _main_deletion_queue.push_function([=]()
                                           { vkDestroyFence(_device, _frames[i]._renderFence, nullptr); });

        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._presentSemaphore));
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));
        _main_deletion_queue.push_function([=]()
                                           {
                vkDestroySemaphore(_device, _frames[i]._presentSemaphore, nullptr);
                vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr); });
    }
    // 注意没有提供VK_FENCE_CREATE_SIGNALED_BIT标识，因为它是立即执行的，而非renderloop中等待其他同步原语
    VkFenceCreateInfo uploadFenceCreateInfo = vkinit::fence_create_info();

    VK_CHECK(vkCreateFence(_device, &uploadFenceCreateInfo, nullptr, &_upload_context._uploadFence));
    _main_deletion_queue.push_function([=]()
                                       { vkDestroyFence(_device, _upload_context._uploadFence, nullptr); });
}

void VenomApp::init_descriptor_pool()
{
    std::vector<VkDescriptorPoolSize> sizes =
        {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, UNIFORM_BUFFERS_CNT},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, UNIFORM_BUFFERS_CNT},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, UNIFORM_BUFFERS_CNT},
            // 着色器读取纹理时将image和sampler一起读取
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, UNIFORM_BUFFERS_CNT},
            // ShadowMap采样器
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, UNIFORM_BUFFERS_CNT},
        };
    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = 0,
        .maxSets = UNIFORM_BUFFERS_CNT,
        .poolSizeCount = (uint32_t)sizes.size(),
        .pPoolSizes = sizes.data()};
    vkCreateDescriptorPool(_device, &pool_info, nullptr, &_descriptor_pool);
}

void VenomApp::init_descriptor_set_layouts()
{
    // set 0
    // binding 0
    VkDescriptorSetLayoutBinding cameraBind = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0);
    // binding 1
    VkDescriptorSetLayoutBinding sceneBind = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
    // binding 2
    VkDescriptorSetLayoutBinding lightSpaceBind = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 2);
    VkDescriptorSetLayoutBinding bindings0[] = {cameraBind, sceneBind, lightSpaceBind};
    VkDescriptorSetLayoutCreateInfo set0info = vkinit::descriptorset_layout_create_info(sizeof(bindings0) / sizeof(VkDescriptorSetLayoutBinding), bindings0);
    vkCreateDescriptorSetLayout(_device, &set0info, nullptr, &_global_set_layout);

    // 使用StorageBuffer存储场景所有对象矩阵
    VkDescriptorSetLayoutBinding objectBind = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
    VkDescriptorSetLayoutCreateInfo set1info = vkinit::descriptorset_layout_create_info(1, &objectBind);
    vkCreateDescriptorSetLayout(_device, &set1info, nullptr, &_object_set_layout);

    // set 2
    // binding 0
    VkDescriptorSetLayoutBinding textureBind = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
    // binding 1
    VkDescriptorSetLayoutBinding shadowBind = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
    VkDescriptorSetLayoutBinding bindings2[] = {textureBind, shadowBind};
    VkDescriptorSetLayoutCreateInfo set2info = vkinit::descriptorset_layout_create_info(sizeof(bindings2) / sizeof(VkDescriptorSetLayoutBinding), bindings2);
    vkCreateDescriptorSetLayout(_device, &set2info, nullptr, &_single_texture_set_layout);

    _main_deletion_queue.push_function([&]()
                                       {
            vkDestroyDescriptorSetLayout(_device, _object_set_layout, nullptr);
            vkDestroyDescriptorSetLayout(_device, _global_set_layout, nullptr);
            vkDestroyDescriptorSetLayout(_device, _single_texture_set_layout, nullptr);

            vkDestroyDescriptorPool(_device, _descriptor_pool, nullptr); });
}

// 创建各类buffer/image/sampler -> 根据set layout为buffer分配decriptor set -> 将buffer/image/sampler写入分配好的descriptor set -> 将上述全部更新到设备
void VenomApp::init_descriptor_sets()
{
    // 多重缓冲共享scene_buffer，因此该buffer不属于FrameData
    const size_t sceneParamBufferSize = MAX_FRAMES_IN_FLIGHT * pad_uniform_buffer_size(sizeof(GPUSceneData));
    _scene_parameter_buffer = create_buffer(sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    
    _light_space_buffer = create_buffer(sizeof(GPULightSpaceData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        // 相机矩阵缓冲 - Uniform Buffer
        _frames[i].cameraBuffer = create_buffer(sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        // allocate one descriptor set for each frame
        VkDescriptorSetAllocateInfo globalSetAlloc = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = _descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &_global_set_layout};
        vkAllocateDescriptorSets(_device, &globalSetAlloc, &_frames[i].globalDescriptorSet);
        VkDescriptorBufferInfo cameraInfo{
            .buffer = _frames[i].cameraBuffer._buffer,
            .offset = 0,
            .range = sizeof(GPUCameraData)};
        VkWriteDescriptorSet cameraWrite = vkinit::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, _frames[i].globalDescriptorSet, &cameraInfo, 0);

        // 物体对象缓冲 - Storage Buffer
        _frames[i].objectBuffer = create_buffer(sizeof(GPUObjectData) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        // allocate the descriptor set that will point to object buffer
        VkDescriptorSetAllocateInfo objectSetAlloc = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = _descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &_object_set_layout};
        vkAllocateDescriptorSets(_device, &objectSetAlloc, &_frames[i].objectDescriptorSet);
        VkDescriptorBufferInfo objectInfo{
            .buffer = _frames[i].objectBuffer._buffer,
            .offset = 0,
            .range = sizeof(GPUObjectData) * MAX_OBJECTS};
        VkWriteDescriptorSet objectWrite = vkinit::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, _frames[i].objectDescriptorSet, &objectInfo, 0);

        //  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER需要手动计算offset：sceneInfo.offset = pad_uniform_buffer_size(sizeof(GPUSceneData)) * i;
        // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC可以不配置offset，在drawcall期间直接变换
        VkDescriptorBufferInfo sceneInfo{
            .buffer = _scene_parameter_buffer._buffer,
            .offset = 0,
            .range = sizeof(GPUSceneData)};
        VkWriteDescriptorSet sceneWrite = vkinit::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, _frames[i].globalDescriptorSet, &sceneInfo, 1);

        VkDescriptorBufferInfo lightSpaceInfo{
            .buffer = _light_space_buffer._buffer,
            .offset = 0,
            .range = sizeof(GPULightSpaceData)
        };
        VkWriteDescriptorSet lightSpaceWrite = vkinit::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, _frames[i].globalDescriptorSet, &lightSpaceInfo, 2);

        VkWriteDescriptorSet setWrites[] = {cameraWrite, sceneWrite, objectWrite, lightSpaceWrite};
        vkUpdateDescriptorSets(_device, sizeof(setWrites) / sizeof(VkWriteDescriptorSet), setWrites, 0, nullptr);
    }
    _main_deletion_queue.push_function([&]()
                                       {
            vmaDestroyBuffer(_allocator, _scene_parameter_buffer._buffer, _scene_parameter_buffer._allocation);
            vmaDestroyBuffer(_allocator, _light_space_buffer._buffer, _light_space_buffer._allocation);
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                vmaDestroyBuffer(_allocator, _frames[i].cameraBuffer._buffer, _frames[i].cameraBuffer._allocation);
                vmaDestroyBuffer(_allocator, _frames[i].objectBuffer._buffer, _frames[i].objectBuffer._allocation);
            } });

    // 纹理采样器
    VkSamplerCreateInfo texturedSamplerInfo = vkinit::sampler_create_info(VK_FILTER_MODE, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);
    VkSampler textureSampler;
    vkCreateSampler(_device, &texturedSamplerInfo, nullptr, &textureSampler);
    _main_deletion_queue.push_function([=]()
                                       { vkDestroySampler(_device, textureSampler, nullptr); });
    // 阴影贴图采样器
    VkSamplerCreateInfo shadowSamplerInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);
    shadowSamplerInfo.magFilter = VK_FILTER_LINEAR;
    shadowSamplerInfo.minFilter = VK_FILTER_LINEAR;
    shadowSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    shadowSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    shadowSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    shadowSamplerInfo.compareEnable = VK_FALSE;  // if true, need extension support
    shadowSamplerInfo.compareOp = VK_COMPARE_OP_LESS;
    shadowSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkSampler shadowSampler;
    vkCreateSampler(_device, &shadowSamplerInfo, nullptr, &shadowSampler);
    _main_deletion_queue.push_function([=]()
                                       { vkDestroySampler(_device, shadowSampler, nullptr); });
    
    // 为已经创建的纹理图像和阴影分配同一DescriptorSet2
    VkDescriptorSetAllocateInfo textureSetAlloc = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = _descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &_single_texture_set_layout};
    Material *texturedMat = __render_system.get_material(_default_material_name); // 创建图形管线时指定
    vkAllocateDescriptorSets(_device, &textureSetAlloc, &texturedMat->textureSet);
    // 将纹理采样器和纹理图像混合在一起，故为COMBINED_IMAGE_SAMPLER bind = 0
    VkDescriptorImageInfo imageBufferInfo{
        .sampler = textureSampler,
        .imageView = _loaded_textures["minecraft"].imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet textureWrite = vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texturedMat->textureSet, &imageBufferInfo, 0);
    
    // 阴影贴图采样器和阴影图像信息 bind = 1
    VkDescriptorImageInfo shadowImageBufferInfo{
        .sampler = shadowSampler,
        .imageView = _shadow_image.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    VkWriteDescriptorSet shadowWrite = vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texturedMat->textureSet, &shadowImageBufferInfo, 1);
    
    VkWriteDescriptorSet writes[] = {textureWrite, shadowWrite};
    vkUpdateDescriptorSets(_device, sizeof(writes) / sizeof(VkWriteDescriptorSet), writes, 0, nullptr);
}

// 初始化Vulkan对象
void VenomApp::initVulkan()
{
    // 初始化Vk实例，可配置项：App名称（用于显卡驱动识别3A做针对性优化）、使用验证层调试、VulkanAPI版本
    vkb::InstanceBuilder builder;

    auto inst_ret = builder.set_app_name(WINDOW_TITLE)
                        .request_validation_layers(true)
                        .require_api_version(1, 1, 0)
                        .use_default_debug_messenger()
                        .build();

    vkb::Instance vkb_inst = inst_ret.value();
    _instance = vkb_inst.instance;
    _debug_messenger = vkb_inst.debug_messenger;

    // 初始化渲染窗口句柄，可采用glfw、SDL库等
    if (glfwCreateWindowSurface(_instance, __window, nullptr, &_surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface!");
    }

    init_device_allocator_queue(vkb_inst);
    init_swapchain();
    init_depth_image();
    init_shadow_map();
    init_command_pool_and_queue();
    init_render_pass();
    init_framebuffers();
    init_syncs();
    init_descriptor_pool();
    init_descriptor_set_layouts();
    init_pipelines(string(SHADER_DIR) + "texture_vert.spv", string(SHADER_DIR) + "texture_frag.spv");

    // 13.创建或者从文件读取纹理、网格数据，分配缓冲区。
    load_texture();
    load_meshes();
    init_descriptor_sets(); // 阴影映射加入了一个采样器（set = 2, binding = 1）

    // ShadowMap功能
    init_shadow_render_pass();
    init_shadow_framebuffer();
    init_shadow_pipeline();

    // 14.通过meshes和materials布置场景
    __render_system.init_scene();

    _is_initialized = true;
}

// 更新渲染资源
void VenomApp::update_render_resource(FrameData &current_frame)
{
    // 场景参数
    _scene_parameters.ambientColor = __main_level.sky_color;
    char *sceneData;
    vmaMapMemory(_allocator, _scene_parameter_buffer._allocation, (void **)&sceneData);
    int frameIndex = _current_frame % MAX_FRAMES_IN_FLIGHT;
    sceneData += pad_uniform_buffer_size(sizeof(GPUSceneData)) * frameIndex;
    memcpy(sceneData, &_scene_parameters, sizeof(GPUSceneData));
    vmaUnmapMemory(_allocator, _scene_parameter_buffer._allocation);

    // 物体对象数据，不使用memcpy是因为类型复杂
    void *objectData;
    vmaMapMemory(_allocator, current_frame.objectBuffer._allocation, &objectData);
    GPUObjectData *objectSSBO = (GPUObjectData *)objectData; // SSBO着色器阶段缓冲对象——可读写，大内存，相对较慢，可运用在所有着色器
    for (int i = 0; i < __renderables.size(); i++)
    {
        if (__renderables[i].mesh == nullptr || __renderables[i].material == nullptr)
            continue;
        objectSSBO[i].modelMatrix = __renderables[i].model_transform; // 【不能为了跳过空的RenderObject而压缩objectSSBO的索引！否则会出现内存位置偏移导致的显示错误！】
        objectSSBO[i].objectLight = __renderables[i].objectLight;
        objectSSBO[i].normal = glm::vec4(__renderables[i].normal, 1.f);
    }
    vmaUnmapMemory(_allocator, current_frame.objectBuffer._allocation);

    // ShadowMap: 更新光照空间数据
    GPULightSpaceData lightSpaceData;
    // 这里需要根据实际情况计算光照空间的视图和投影矩阵
    // 示例：假设光源位置和方向
    glm::vec3 lightPos = glm::vec3(10.0f, 10.0f, 10.0f);
    glm::vec3 lightDir = glm::normalize(-lightPos);
    lightSpaceData.lightView = glm::lookAt(lightPos, lightPos + lightDir, glm::vec3(0.0f, 1.0f, 0.0f));
    lightSpaceData.lightProj = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 50.0f);

    void *lightSpaceDataPtr;
    vmaMapMemory(_allocator, _light_space_buffer._allocation, &lightSpaceDataPtr);
    memcpy(lightSpaceDataPtr, &lightSpaceData, sizeof(GPULightSpaceData));
    vmaUnmapMemory(_allocator, _light_space_buffer._allocation);
}

// 将更新数据拷贝/映射到缓冲区 -> (管线变化时)重新绑定材质对应的管线及描述符集合
void VenomApp::draw_objects(VkCommandBuffer cmd, FrameData &current_frame)
{
    // 现在用StorageBuffer传输所有物体的矩阵到GPU，下面代码逐物体传输pushconstants严重影响帧率【过时】
    //            MeshPushConstants constants;
    //            constants.render_matrix = object.model_transform;
    //            vkCmdPushConstants(cmd, object.material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

    // 上传光源主视角和光照矩阵到显存，将大量的矩阵相乘交给GPU， 用shader计算矩阵相乘结果
    GPUCameraData cameraData;
    __main_camera.set_camera_vp_matrices(cameraData);
    void *data;
    vmaMapMemory(_allocator, current_frame.cameraBuffer._allocation, &data);
    memcpy(data, &cameraData, sizeof(GPUCameraData));
    vmaUnmapMemory(_allocator, current_frame.cameraBuffer._allocation);

    // 纹理数据
    Mesh *lastMesh = nullptr;
    Material *lastMaterial = nullptr;
    for (int i = 0; i < __renderables.size(); i++)
    {
        if (__renderables[i].mesh == nullptr || __renderables[i].material == nullptr)
            continue;
        RenderObject &object = __renderables[i];
        // 只有当要渲染的材质和上一个不同时，才要重新绑定管线；管线不同时，也要重新绑定DescriptorSet
        if (object.material != lastMaterial)
        {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
            lastMaterial = object.material;
            // bind the descriptor set when changing pipeline
            // offset for our scene buffer
            uint32_t dynamic_uniform_offset = pad_uniform_buffer_size(sizeof(GPUSceneData)) * (_current_frame % MAX_FRAMES_IN_FLIGHT);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1, &current_frame.globalDescriptorSet, 1, &dynamic_uniform_offset);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 1, 1, &current_frame.objectDescriptorSet, 0, nullptr);
            if (object.material->textureSet != VK_NULL_HANDLE)
            {
                // texture descriptor
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 2, 1, &object.material->textureSet, 0, nullptr);
            }
        }

        // 只有和上次的VertexBuffer不同时，才需要重新绑定
        if (object.mesh != lastMesh)
        {
            // bind the mesh vertex buffer with offset 0
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->_vertexBuffer._buffer, &offset);
            lastMesh = object.mesh;
        }
        // vkCmdDraw后四个参数分别对应 vertexCount instanceCount firstVertex firstInstance->对应到vert shader的gl_BaseInstance
        vkCmdDraw(cmd, object.mesh->_vertices.size(), 1, 0, i);
    }
}

// 执行一次绘制调用
void VenomApp::drawcall()
{
    FrameData current_frame = get_current_frame();
    update_render_resource(current_frame);
    // 1.等待GPU完成上一帧渲染，1s（1e9ns）超时丢弃；如果设置0s超时丢弃，可用于判断GPU是否在执行指令
    VK_CHECK(vkWaitForFences(_device, 1, &current_frame._renderFence, true, 1e9));
    // 2.重置ResetFence以供本帧对CPU加锁
    VK_CHECK(vkResetFences(_device, 1, &current_frame._renderFence));
    // 3.GPU从Swapchain取一帧，注意持有了一个信号量，1s超时
    uint32_t swapchainImageIndex;
    VkResult acquire = vkAcquireNextImageKHR(_device, _swapchain, 1e9, current_frame._presentSemaphore, nullptr, &swapchainImageIndex);
    if (acquire == VK_ERROR_OUT_OF_DATE_KHR)
    { // 窗口大小被重置，导致原来的Swapchain超时
        _resize_requested = true;
        return;
    }
    // 4.清空CmdBuffer
    VkCommandBuffer cmd = current_frame._mainCommandBuffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0));
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

    // 6.启动RenderPass，加入绘制指令
    // 【开放接口】清空内容的刷新画面（也就是天空）
    VkClearValue clearValue;
    __main_level.update_sky_color();
    clearValue.color = {{__main_level.sky_color.x,
                         __main_level.sky_color.y,
                         __main_level.sky_color.z}};

    // clear depth at 1
    VkClearValue depthClear;
    depthClear.depthStencil.depth = 1.f;

    // connect clear values
    VkClearValue clearValues[] = {clearValue, depthClear};

    VkRenderPassBeginInfo rpInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = _render_pass,
        .framebuffer = _framebuffers[swapchainImageIndex],
        .renderArea = {
            .offset = {
                .x = 0, .y = 0},
            .extent = _window_extent},
        .clearValueCount = 2,
        .pClearValues = clearValues};

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    // 【开放接口】7.RenderPass中执行：绑定渲染管线到cmd，执行绘制等指令
    draw_objects(cmd, current_frame);
    // 8.结束RenderPass
    vkCmdEndRenderPass(cmd);
    // finalize the command buffer (we can no longer add commands, but it can now be executed)
    VK_CHECK(vkEndCommandBuffer(cmd));

    // 9.提交Cmd到VkQueue，让GPU执行
    // prepare the submission to the queue.
    // we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
    // we will signal the _renderSemaphore, to signal that rendering has finished
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,                               // 等第3步GPU从swapchain取完NextImage
        .pWaitSemaphores = &(current_frame._presentSemaphore), // 发送到GPU开始渲染
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &(current_frame._renderSemaphore)};
    // submit command buffer to the queue and execute it.
    //  renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit(_graphics_queue, 1, &submit, current_frame._renderFence));

    // 10.等渲染完成后，将GPU渲染的结果到窗口
    // this will put the image we just rendered into the visible window.
    // we want to wait on the renderSemaphore for that,
    // as it's necessary that drawing commands have finished before the image is displayed to the user
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &(current_frame._renderSemaphore),
        .swapchainCount = 1,
        .pSwapchains = &_swapchain,
        .pImageIndices = &swapchainImageIndex};
    VkResult present = vkQueuePresentKHR(_graphics_queue, &presentInfo);
    if (present == VK_ERROR_OUT_OF_DATE_KHR)
    { // 窗口大小被重置，导致超时
        _resize_requested = true;
    }

    ++_current_frame;
}

// 渲染循环
void VenomApp::mainLoop()
{
    while (!glfwWindowShouldClose(__window))
    {                     // 窗口即将关闭
        glfwPollEvents(); // 等待OS的事件队列
        get_input();      // 输入按键、鼠标等操作
        drawcall();
        if (_resize_requested)
        {
            resize_swapchain();
        }
        __physics_system.simulate(1); // 物理模拟，传入该渲染帧时间间隔
        tok(_current_frame);
    }
    vkDeviceWaitIdle(_device); // 等待设备资源不再需求时结束
}

// 结束渲染，释放资源
void VenomApp::cleanup()
{
    if (_is_initialized)
    {
        vkDeviceWaitIdle(_device);
        _main_deletion_queue.flush();
        vkDestroySurfaceKHR(_instance, _surface, nullptr); // 销毁渲染界面一定要在销毁实例之前！
        vkDestroyDevice(_device, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
        vkDestroyInstance(_instance, nullptr); // 会将物理设备一起清理
        glfwDestroyWindow(__window);
        glfwTerminate();
    }
}

// ShadowMap功能

// 初始化阴影贴图和图像视图
void VenomApp::init_shadow_map()
{
    // 创建阴影贴图图像
    VkImageCreateInfo shadowImageInfo = vkinit::image_create_info(
        VK_FORMAT_D32_SFLOAT, // 深度格式
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        _depth_extent
    );
    VmaAllocationCreateInfo shadowImageAllocInfo = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };
    vmaCreateImage(_allocator, &shadowImageInfo, &shadowImageAllocInfo, &_shadow_image.image, &_shadow_image.allocation, nullptr);

    // 创建阴影贴图图像视图
    VkImageViewCreateInfo shadowImageViewInfo = vkinit::image_view_create_info(
        VK_FORMAT_D32_SFLOAT,
        _shadow_image.image,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );
    VK_CHECK(vkCreateImageView(_device, &shadowImageViewInfo, nullptr, &_shadow_image.imageView));

    _main_deletion_queue.push_function([=]()
                                       {
            vkDestroyImageView(_device, _shadow_image.imageView, nullptr);
            vmaDestroyImage(_allocator, _shadow_image.image, _shadow_image.allocation);
        });

    immediate_submit([&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        };

        // 先转换到深度模板附件最优布局
        VkImageMemoryBarrier imageBarrier_toDepth = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .image = _shadow_image.image,
            .subresourceRange = range,
        };
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toDepth);

        // 再转换到着色器只读最优布局
        VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toDepth;
        imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageBarrier_toReadable.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);
    });
}

// 初始化阴影渲染通道
void VenomApp::init_shadow_render_pass()
{
    VkAttachmentDescription shadow_attachment = {
        .format = VK_FORMAT_D32_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkAttachmentReference shadow_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 0,
        .pDepthStencilAttachment = &shadow_attachment_ref
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &shadow_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_shadow_render_pass));
    _main_deletion_queue.push_function([=]()
                                       { vkDestroyRenderPass(_device, _shadow_render_pass, nullptr); });
}

// 初始化阴影帧缓冲和深度贴图
void VenomApp::init_shadow_framebuffer()
{
    VkExtent2D shadow_extent = {2048, 2048};

    VkImageCreateInfo shadow_image_info = vkinit::image_create_info(VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, {shadow_extent.width, shadow_extent.height, 1});
    VmaAllocationCreateInfo shadow_alloc_info = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };

    vmaCreateImage(_allocator, &shadow_image_info, &shadow_alloc_info, &_shadow_image.image, &_shadow_image.allocation, nullptr);

    VkImageViewCreateInfo shadow_view_info = vkinit::image_view_create_info(VK_FORMAT_D32_SFLOAT, _shadow_image.image, VK_IMAGE_ASPECT_DEPTH_BIT);
    VK_CHECK(vkCreateImageView(_device, &shadow_view_info, nullptr, &_shadow_image.imageView));

    VkFramebufferCreateInfo framebuffer_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = _shadow_render_pass,
        .attachmentCount = 1,
        .pAttachments = &_shadow_image.imageView,
        .width = shadow_extent.width,
        .height = shadow_extent.height,
        .layers = 1
    };

    VK_CHECK(vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &_shadow_framebuffer));

    _main_deletion_queue.push_function([=]()
                                       {
            vkDestroyFramebuffer(_device, _shadow_framebuffer, nullptr);
            vkDestroyImageView(_device, _shadow_image.imageView, nullptr);
            vmaDestroyImage(_allocator, _shadow_image.image, _shadow_image.allocation);
        });
}

// 初始化阴影渲染管线
void VenomApp::init_shadow_pipeline()
{
    VkShaderModule vertShader;
    if (!load_shader_module(string(SHADER_DIR) + "texture_vert.spv", &vertShader))
    {
        std::cout << "Error when building the shadow vertex shader module" << std::endl;
    }

    VkPipelineLayout shadowPipelineLayout;
    VkDescriptorSetLayout shadowSetLayouts[] = {_global_set_layout, _object_set_layout};
    VkPipelineLayoutCreateInfo shadow_pipeline_layout_info = vkinit::pipeline_layout_create_info();
    shadow_pipeline_layout_info.setLayoutCount = 2;
    shadow_pipeline_layout_info.pSetLayouts = shadowSetLayouts;
    VK_CHECK(vkCreatePipelineLayout(_device, &shadow_pipeline_layout_info, nullptr, &shadowPipelineLayout));

    VkPipeline shadowPipeline;
    PipelineBuilder pipelineBuilder;

    pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
    VertexInputDescription vertexDescription = Vertex::get_vertex_description();
    pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
    pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();
    pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
    pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

    pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    pipelineBuilder._viewport.x = 0.0f;
    pipelineBuilder._viewport.y = 0.0f;
    pipelineBuilder._viewport.width = (float)_window_extent.width;
    pipelineBuilder._viewport.height = (float)_window_extent.height;
    pipelineBuilder._viewport.minDepth = 0.0f;
    pipelineBuilder._viewport.maxDepth = 1.0f;
    pipelineBuilder._scissor.offset = {0, 0};
    pipelineBuilder._scissor.extent = _window_extent;

    pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, false);
    pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();
    // pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_enable_state(VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD);
    pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS);

    pipelineBuilder._shaderStages.clear();
    pipelineBuilder._shaderStages.push_back(
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertShader));
    pipelineBuilder._pipelineLayout = shadowPipelineLayout;

    shadowPipeline = pipelineBuilder.build(_device, _shadow_render_pass);

    vkDestroyShaderModule(_device, vertShader, nullptr);
    _main_deletion_queue.push_function([=]()
                                       {
            vkDestroyPipeline(_device, shadowPipeline, nullptr);
            vkDestroyPipelineLayout(_device, shadowPipelineLayout, nullptr);
        });
}
