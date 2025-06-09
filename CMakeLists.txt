cmake_minimum_required(VERSION 3.10)

# 项目名称
project(Venom)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 去掉警告信息
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")


if(APPLE)
    # 查找 Vulkan SDK 的头文件目录
    set(VULKAN_SDK_BASE_DIR ~/VulkanSDK)
    file(TO_CMAKE_PATH "${VULKAN_SDK_BASE_DIR}" VULKAN_SDK_BASE_DIR)

    # 使用 shell 命令获取最新的 Vulkan SDK 版本号目录
    execute_process(
        COMMAND bash -c "ls -d ${VULKAN_SDK_BASE_DIR}/1.* | sort -V | tail -n 1"
        OUTPUT_VARIABLE VULKAN_SDK_VERSION_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT VULKAN_SDK_VERSION_DIR)
        message(FATAL_ERROR "Vulkan SDK directory not found")
    endif()

    # 设置包含文件的目录路径
    set(VULKAN_SDK_INCLUDE_DIR "${VULKAN_SDK_VERSION_DIR}/macOS/include")
    include_directories(${VULKAN_SDK_INCLUDE_DIR})
endif()

# 头文件目录
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(GAME_CORE_DIR ${SRC_DIR}/core)
set(MATH_DIR ${SRC_DIR}/math)
set(VULKAN_DIR ${SRC_DIR}/vulkan)
set(THIRD_PARTY_DIR ${SRC_DIR}/third_party)
include_directories(${GAME_CORE_DIR})
include_directories(${MATH_DIR})
include_directories(${VULKAN_DIR})
include_directories(${THIRD_PARTY_DIR})

# 添加源文件
add_executable(Venom 
    ${THIRD_PARTY_DIR}/VkBootstrap.cpp
    ${VULKAN_DIR}/vk_engine.hpp
    ${SRC_DIR}/main.cpp
    )

if(APPLE)
    # 查找 Vulkan SDK 的库文件目录
    set(VULKAN_SDK_LIBRARY_DIR "${VULKAN_SDK_VERSION_DIR}/macOS/lib")
    find_library(VULKAN_SDK_LIBRARY NAMES vulkan PATHS ${VULKAN_SDK_LIBRARY_DIR})
    if(NOT VULKAN_SDK_LIBRARY)
        message(FATAL_ERROR "Vulkan library not found")
    endif()
    target_link_libraries(Venom ${VULKAN_SDK_LIBRARY})

    # 使用 shell 命令获取最新的 GLFW 版本号目录
    execute_process(
        COMMAND bash -c "ls -d /opt/homebrew/Cellar/glfw/*/ | sort -V | tail -n 1"
        OUTPUT_VARIABLE GLFW_VERSION_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT GLFW_VERSION_DIR)
        message(FATAL_ERROR "GLFW directory not found")
    endif()

    # 设置 GLFW 的库文件目录
    set(GLFW_LIBRARY_DIR "${GLFW_VERSION_DIR}/lib")
    find_library(GLFW_LIBRARY NAMES libglfw.dylib PATHS ${GLFW_LIBRARY_DIR})
    if(NOT GLFW_LIBRARY)
        message(FATAL_ERROR "GLFW library not found")
    endif()
    target_link_libraries(Venom ${GLFW_LIBRARY})

endif()