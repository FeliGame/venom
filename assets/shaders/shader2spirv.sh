#!/bin/bash

# 切换到脚本所在目录
cd "$(dirname "$0")"

# 设置 Vulkan SDK 的基础目录
VULKAN_SDK_BASE_DIR="$HOME/VulkanSDK"

# 使用 ls 命令获取最新的 Vulkan SDK 版本号目录
VULKAN_SDK_VERSION_DIR=$(ls -d "${VULKAN_SDK_BASE_DIR}"/1.* | sort -V | tail -n 1)

# 检查是否找到了版本号目录
if [ -z "$VULKAN_SDK_VERSION_DIR" ]; then
    echo "Vulkan SDK directory not found"
    exit 1
fi

# 设置 glslc 编译器的路径
GLSLC="${VULKAN_SDK_VERSION_DIR}/macOS/bin/glslc"

# 检查 glslc 编译器是否存在
if [ ! -f "$GLSLC" ]; then
    echo "glslc compiler not found at ${GLSLC}"
    exit 1
fi

# 查找所有 .vert 和 .frag 文件
shader_files=$(find . -type f \( -name "*.vert" -o -name "*.frag" \))

# 初始化编译失败标志
compile_failed=false

for shader in $shader_files; do
    spv_file="${shader%.*}_${shader##*.}.spv"
    # 检查对应的 .spv 文件是否存在
    if [ ! -f "$spv_file" ]; then
        echo "Compiling $shader to $spv_file"
        $GLSLC "$shader" -o "$spv_file"
        if [ $? -ne 0 ]; then
            echo "Compilation of $shader failed"
            compile_failed=true
        fi
    fi
done

# 检查编译是否有失败的
if [ "$compile_failed" = true ]; then
    echo "Shaders Compilation failed"
    exit 1
fi
