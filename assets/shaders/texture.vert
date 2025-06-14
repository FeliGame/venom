#version 450
#extension GL_ARB_separate_shader_objects: enable
// 输入顶点缓冲区数据（Vertex）
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;
layout (location = 3) in vec2 vTexCoord;

layout (location = 0) out vec4 objectLight;
layout (location = 1) out vec2 texCoord;
layout (location = 2) out float z;    // 屏幕中心到目标面渲染距离
layout (location = 3) out vec4 fragPosLightSpace; // ShadowMap：顶点在光照空间的位置
layout (location = 4) out vec3 fragNormal;

layout(set = 0, binding = 0) uniform  CameraBuffer{
    mat4 view;
    mat4 proj;
    vec4 camPos;    // 相机位置
} cameraData; // 顶点着色器只需要前四项

// ShadowMap：光照空间的视图和投影矩阵
layout(set = 0, binding = 2) uniform LightSpaceBuffer {
    mat4 lightView;
    mat4 lightProj;
} lightSpaceData;

struct ObjectData{
    mat4 model;
    vec4 objectLight;  // 物体自身光强，xyz为自身光照色彩，w所在格亮度（1.0满亮度）
    vec4 normal;
};

//all object matrices, std140 是内存布局规则，使用它可以与C++的数组运行模式匹配；set = 1对应区别于camera使用的另一个DescriptorSet；readonly buffer告知这是一个只读的buffer
layout(std140,set = 1, binding = 0) readonly buffer ObjectBuffer{
    ObjectData objects[];
} objectBuffer;

void main()
{
    const mat4 modelMatrix = objectBuffer.objects[gl_InstanceIndex].model;
    const mat4 vm = cameraData.view * modelMatrix;
    const vec4 viewPos = vm * vec4(vPosition, 1.0f);
    gl_Position = cameraData.proj * viewPos;
    texCoord = vTexCoord;
    z = viewPos.z / viewPos.w;

    // ShadowMap
    objectLight = objectBuffer.objects[gl_InstanceIndex].objectLight;
    // 计算顶点在光照空间的位置
    fragPosLightSpace = lightSpaceData.lightProj * lightSpaceData.lightView * modelMatrix * vec4(vPosition, 1.0);
    // 传递法线信息，进行模型矩阵变换并归一化
    fragNormal = normalize(mat3(transpose(inverse(modelMatrix))) * vNormal); 
}
