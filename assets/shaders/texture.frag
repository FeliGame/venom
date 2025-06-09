#version 450

//shader input
layout (location = 0) in vec4 objectLight; // 物体自身光强，xyz为自身光照色彩，w所在格亮度（1.0满亮度）
layout (location = 1) in vec2 texCoord;
layout (location = 2) in float z;	// 屏幕中心到目标面渲染距离，越远越负


//output write
layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform  CameraBuffer{
    mat4 view;
    mat4 proj;
    vec4 camPos;    // 相机位置
} cameraData;
layout(set = 0, binding = 1) uniform  SceneData{
    vec4 fogColor; // w is for exponent
    vec4 fogDistance; //x for min, y for max, z 越大雾气扩大速度越慢, w unused.
    vec4 ambientColor;
    vec4 sunPos; //w for sun power
    vec4 sunlightColor; // w为天空颜色反射率（决定方块整体反射天空颜色的程度）
} sceneData;
layout(set = 2, binding = 0) uniform sampler2D textureSampler;

void main()
{
    vec4 texColor = texture(textureSampler,texCoord);
    float fog_min = sceneData.fogDistance.x; // 烟雾最近距离
    float fogDensity = min(max(-(fog_min + z) / (fog_min - sceneData.fogDistance.z * z), 0.0f), 1.0f); // 以玩家位置推算烟雾浓度，越远烟雾越浓
    vec3 color = (texColor.xyz + sceneData.fogColor.xyz * fogDensity) * sceneData.ambientColor.xyz;	// 材质包原色理解成是#FFFFFF下的反射效果，乘以天空颜色相当于按天空光缩小亮度
    outFragColor = vec4(color, texColor.w);
}
