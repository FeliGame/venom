// 管理渲染管线和material、对象创建和场景绘制

#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <level_system.h>
#include <unordered_map>
#include <vk_types.h>
#include <player.h>

static const int CHUNK_RENDER_RADIUS = 3;       // 渲染玩家附近多少区块
static const int CHUNK_GEN_RADIUS = 4;          // 创建世界时渲染玩家附近多少区块
static const int CHUNK_RENDER_COUNT_MAX = 2e2;  // 区块最大渲染数量
static const int REFRESH_RENDER_FACE_MAX = 1e5; // 触发全局刷新的最大渲染面数
static const int TEXTURE_SIZE = 16;             // 正方形材质包边长
static const int FACE_F = 0, FACE_R = 1, FACE_U = 2, FACE_D = 3, FACE_L = 4, FACE_B = 5;                     // 方块的六个面对应的索引
static const glm::vec3 FACE_NORMALS[6]{{0, -1, 0}, {1, 0, 0}, {0, 0, 1}, {0, 0, -1}, {-1, 0, 0}, {0, 1, 0}}; // 方块的六个面对应的法线

// default array of renderable objects
extern std::vector<RenderObject> __renderables;
extern std::vector<Chunk *> __rendered_chunks; // 已渲染的区块列表
// 每个可渲染对象所需的材质和网格来自于下列属性
extern std::unordered_map<std::string, Material> __materials;
extern std::vector<Mesh> __meshes; // 第一格为空【GPU会吞掉第一格的贴图，原因未知】

class RenderSystem
{
private:
    // 以下属性用来将面组装成一个Block，遵循面顺序： FRUDLB
    const glm::vec3 BLOCK_TRANSLATE_DIST[6] = {
        glm::vec3(0, 0, 0),
        glm::vec3(0, 0, 1),
        glm::vec3(0, 1, 0),
        glm::vec3(0, 0, 1),
        glm::vec3(1, 0, 0),
        glm::vec3(1, 0, 1),
    }; // 六个面的平移距离

    const glm::mat4 BLOCK_ROTATION[6] = {
        glm::rotate(glm::mat4{1.0}, glm::radians(0.0f), glm::vec3(1, 0, 0)),
        glm::rotate(glm::mat4{1.0}, glm::radians(90.0f), glm::vec3(0, 1, 0)),
        glm::rotate(glm::mat4{1.0}, glm::radians(90.0f), glm::vec3(1, 0, 0)),
        glm::rotate(glm::mat4{1.0}, glm::radians(-90.0f), glm::vec3(1, 0, 0)),
        glm::rotate(glm::mat4{1.0}, glm::radians(-90.0f), glm::vec3(0, 1, 0)),
        glm::rotate(glm::mat4{1.0}, glm::radians(180.0f), glm::vec3(0, 1, 0)),
    }; // 六个面旋转方式

    const glm::ivec3 BLOCK_DIR[6] = {
        glm::ivec3(0, 0, -1), // F
        glm::ivec3(-1, 0, 0), // R
        glm::ivec3(0, 1, 0),  // U
        glm::ivec3(0, -1, 0), // D
        glm::ivec3(1, 0, 0),  // L
        glm::ivec3(0, 0, 1)   // B
    }; // 相对Block的六个面的相邻单位向量
public:
    Material *DEFAULT_MATERIAL;

    Material *create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string &name);
    Material *get_material(const std::string &name);
    Mesh *get_mesh(BLOCK_ENUM block_kind);
    void init_scene();
    void render_face(Block *block, int i);
    void unrender_face(Block *block, int i);
    void render_block(Block *block);
    void unrender_block(glm::ivec3 pos);
    void render_chunk(int rx, int ry, int rz);
    void unrender_chunk(Chunk *chunk);
    void render_all_chunks(glm::vec3 player_pos, bool rerender);
    void update_render_chunks(glm::ivec3 chunk_pos, glm::vec3 player_pos);
};

static RenderSystem __render_system; // 场景、对象管理器

#endif /* RENDER_SYSTEM_H */
