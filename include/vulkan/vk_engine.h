#ifndef VK_ENGINE_H
#define VK_ENGINE_H

#define SHADER_DIR "./assets/shaders/"
#define TEXTURE_DIR "./assets/textures/"

#include <vk_init.h>
#include <physics_system.h>
#include <input_system.h>
#include <cmath>

// #define VK_USE_PLATFORM_METAL_EXT
// #define GLFW_INCLUDE_VULKAN //GLFW已包含vulkan.h

#include <VkBootstrap.h> // VkBootstrap是一个Vulkan初始化封装库，可以简化开发，https://github.com/charles-lunarg/vk-bootstrap

// 顶点缓冲需要分配内存（AMD的VMA库）
#include <vma/vk_mem_alloc.h>

#include <GLFW/glfw3.h> //负责前端窗口管理
#include <GLFW/glfw3native.h>
#include <algorithm>
#include <limits>
#include <iostream>
#include <stdexcept>
#include <cstdlib> //提供EXIT宏定义
#include <cstdint>
#include <cstring>
#include <vector>
#include <map>
#include <optional>
#include <set>
#include <fstream>
#include <unordered_map>
#include <deque>
#include <cmath>
#include <functional>

#include <stb_image.h>

// 用于计算帧率
static std::chrono::high_resolution_clock::time_point clock_early, clock_late;
static uint32_t frame_start;
inline void tik(uint32_t current_frame)
{
    clock_early = std::chrono::high_resolution_clock::now();
    frame_start = current_frame;
}
static const float FPS_CHECK_PERIOD = 1; // 检测间隔（单位：秒）
inline void tok(uint32_t current_frame)
{
    clock_late = std::chrono::high_resolution_clock::now();
    double elapse_sec = (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(clock_late - clock_early).count()) / 1e9;
    if (elapse_sec >= FPS_CHECK_PERIOD)
    {
        clock_early = clock_late;
        string title = WINDOW_TITLE + string(" FPS: ") + to_string(current_frame - frame_start);
        glfwSetWindowTitle(__window, title.c_str());
        frame_start = current_frame;
    }
}

// 以下是光栅化的功能选项
#define VK_POLYGON_MODE VK_POLYGON_MODE_FILL // 多面体呈现模式
// VK_POLYGON_MODE_FILL
// VK_POLYGON_MODE_LINE
// VK_POLYGON_MODE_POINT
// VK_POLYGON_MODE_FILL_RECTANGLE_NV
#define VK_CULL_MODE VK_CULL_MODE_BACK_BIT // 正/背面剔除模式
// VK_CULL_MODE_NONE
// VK_CULL_MODE_FRONT_BIT
// VK_CULL_MODE_BACK_BIT
// VK_CULL_MODE_FRONT_AND_BACK

// 深度测试
#define VK_COMPARE_OP VK_COMPARE_OP_LESS // 如何处理深度差（较近的覆盖较远的）

// 纹理过滤/映射模式
#define VK_FILTER_MODE VK_FILTER_NEAREST
// VK_FILTER_NEAREST，取最近点像素，最快
// VK_FILTER_LINEAR，取附近两个点做线性插值
#define VK_SHADOW_FILTER_MODE VK_FILTER_LINEAR

using namespace std;

// 报错即终止
#define VK_CHECK(x)                                                     \
    do                                                                  \
    {                                                                   \
        VkResult err = x;                                               \
        if (err)                                                        \
        {                                                               \
            std::cout << "Detected Vulkan error: " << err << std::endl; \
            abort();                                                    \
        }                                                               \
    } while (0)

const int MAX_FRAMES_IN_FLIGHT = 2; // 一次性加载的帧数（不会因为渲染当前帧而打扰下一帧的录入，选2可以防止CPU过于超过GPU进度）
const int UNIFORM_BUFFERS_CNT = 10;
const int MAX_OBJECTS = 1e6; // 每帧最大渲染物体数量，虚幻引擎采用动态扩展上限技术（场景物体越多，该数值越大）
// 目前的代码逻辑下，如果该数值少于场景内物体数量且开启多重缓冲，则会发生屏闪（猜测是因为每次缓冲的对象集合有所不同）；如果缓冲数为1，则渲染的网格关系错乱（猜测是丢失了一些顶点数据）

// add the include for glm to get matrices
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()> &&function)
    {
        deletors.push_back(function);
    }

    void flush()
    {
        // reverse iterate the deletion queue to execute all the functions
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
        {
            (*it)(); // call functors
        }

        deletors.clear();
    }
};

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

class VenomApp
{
public:
    void run();

private:
    DeletionQueue _main_deletion_queue;
    bool _is_initialized = false;
    uint32_t _current_frame = 0;

    bool _framebuffer_resized = false; // VK_ERROR_OUT_OF_DATE_KHR 不一定被触发，需要显式切换保证
    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_messenger; // 调试信息（用于回调）,启用验证层时使用

    VkPhysicalDevice _physical_device = VK_NULL_HANDLE; // 物理设备（GPU）
    VkPhysicalDeviceProperties _gpu_properties;
    VkDevice _device; // 逻辑设备，注意和实例没有直接关联！
    VkQueue _graphics_queue;
    uint32_t _graphics_queue_family; // 图形队列簇（记录的是索引！）

    VkSurfaceKHR _surface; // 调用窗口渲染，离屏渲染可以不使用

    VkSwapchainKHR _swapchain;
    vector<VkImage> _swapchain_images;                            // swap chain缓冲区内图像
    VkFormat _swapchain_image_format;                             // swap chain缓冲模式
    VkExtent2D _window_extent{MAX_WIDTH << 1, MAX_HEIGHT << 1};   // 渲染窗口尺寸
    VkExtent3D _depth_extent{MAX_WIDTH << 1, MAX_HEIGHT << 1, 1}; // 深度图多出一个z轴
    vector<VkImageView> _swapchain_image_views;                   // image的包装器，可用于换颜色
    vector<VkFramebuffer> _framebuffers;                          // 帧缓冲

    VkCommandPool _command_pool; // 指令池（绘制、内存操作、运算）

    VkCommandBuffer _main_command_buffer; // 每一帧需要自己的指令缓冲区（二缓、三缓），这里暂时只分配一个即可

    FrameData _frames[MAX_FRAMES_IN_FLIGHT];

    // Chapter 3 顶点缓冲区
    VmaAllocator _allocator; // vma lib allocator

    VkRenderPass _render_pass;

    // ImageView是Image的包装器，这里我们自己初始化，而不使用VkBootstrap
    AllocatedImage _depth_image;

    VkDescriptorSetLayout _global_set_layout;
    VkDescriptorSetLayout _object_set_layout;
    VkDescriptorSetLayout _single_texture_set_layout;
    VkDescriptorPool _descriptor_pool;

    // 场景内全部物体均受影响
    GPUSceneData _scene_parameters;
    AllocatedBuffer _scene_parameter_buffer;

    UploadContext _upload_context;

    unordered_map<string, Texture> _loaded_textures; // 图形渲染管线及其布局藏在这！
    std::string _default_material_name = "textured"; // 默认材质的名称

    bool _resize_requested = false; // 窗口大小重置

    void resize_swapchain();
    void immediate_submit(std::function<void(VkCommandBuffer cmd)> &&function);
    size_t pad_uniform_buffer_size(size_t originalSize);
    AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    FrameData &get_current_frame();
    void load_texture();
    void load_meshes();
    bool load_from_image(const char *file, AllocatedImage &outImage);
    void upload_mesh(Mesh &mesh);
    static void framebufferResizeCallback(GLFWwindow *window, int width, int height)
    {                                                                              // 更新FrameBuffer尺寸时的回调
        auto app = reinterpret_cast<VenomApp *>(glfwGetWindowUserPointer(window)); // 抓取伪指针
        app->_framebuffer_resized = true;
    }
    void initWindow();
    bool load_shader_module(const string &filePath, VkShaderModule *outShaderModule);
    void init_pipelines(const string &shader_vert_path, const string &shader_frag_path);
    void init_device_allocator_queue(vkb::Instance &vkb_inst);
    void init_swapchain();
    void init_depth_image();
    void init_command_pool_and_queue();
    void init_render_pass();
    void init_framebuffers();
    void init_syncs();
    void init_descriptor_pool();
    void init_descriptor_set_layouts();
    void init_descriptor_sets();
    void initVulkan();
    void update_render_resource(FrameData &current_frame);
    void draw_objects(VkCommandBuffer cmd, FrameData &current_frame);
    void drawcall();
    void mainLoop();
    void cleanup();
};

#endif // VK_ENGINE_H