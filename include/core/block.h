#ifndef BLOCK_H
#define BLOCK_H

#include <vector>
#include <vk_types.h>
#include <vk_mesh.h>
#include <unordered_map>

// 【枚举值排列必须严格遵照材质包顺序（先行后列）】
enum BLOCK_ENUM
{
    // BLOCK_NULL是无操作，生成世界时使用；RENDER开头的枚举值仅供渲染时使用
    BLOCK_NULL = -1,
    BLOCK_AIR = 181,
    //    line
    //    BLOCK_, BLOCK_, BLOCK_, BLOCK_, BLOCK_, BLOCK_, BLOCK_, BLOCK_, BLOCK_, BLOCK_, BLOCK_, BLOCK_, BLOCK_, BLOCK_, BLOCK_, BLOCK_,

    // line 1
    BLOCK_CLAY = 0,
    BLOCK_STONE,
    BLOCK_DIRT,
    BLOCK_GRASS,
    BLOCK_OAK_SLAB,
    BLOCK_DOUBLE_STONE_SLAB,
    BLOCK_232,
    BLOCK_BRICK,
    BLOCK_TNT,
    BLOCK_TNT_TOP,
    BLOCK_TNT_BOTTOM,
    BLOCK_SPIDER_WEB,
    BLOCK_ROSE,
    BLOCK_YELLOW_FLOWER,
    BLOCK_WATER,
    BLOCK_OAK_SAPLING,
    // line 2
    BLOCK_COBBLE_STONE,
    BLOCK_BEDROCK,
    BLOCK_24,
    BLOCK_123,
    BLOCK_43,
    BLOCK_5,
    BLOCK_2,
    BLOCK_55,
    BLOCK_455,
    BLOCK_4,
    BLOCK_3,
    BLOCK_46,
    BLOCK_513,
    BLOCK_34,
    BLOCK_45,
    BLOCK_54,
    // line3
    BLOCK_GOLD_ORE,
    BLOCK_IRON_ORE,
    BLOCK_COAL_ORE,
    BLOCK_BOOK_SHELF,
    BLOCK_MOSS_STONE,
    BLOCK_OBSIDIAN,
    BLOCK_GRASS_TRANSPARENT,
    BLOCK_GRASS_ENTITY,
    RENDER_GRASS_TOP
};

const static int FACE_UNRENDERED = -1; // 表示方块的这个面没有被渲染，由于memset，只能是-1

// 世界的基本单位是Block
class Block
{
private:
    Block() {}

public:
    glm::ivec3 pos;        // 所处坐标
    RenderObject faces[6]; // 6个渲染面依次是frubld，在不可直视面渲染时，可以裁剪掉这部分面以提升性能
    int face_id[6];
    float brightness = LIGHT_SCALE; // 该方块的光照强度
    BLOCK_ENUM kind;                // 种类id
    bool transparent;               // 是否透明

    Block(int p_x, int p_y, int p_z, BLOCK_ENUM p_kind);
    Block(glm::ivec3 p_pos, BLOCK_ENUM p_kind);
};

// 【模型导入】颜色->Block种类映射关系
extern std::unordered_map<unsigned, BLOCK_ENUM> __argb2block; // 声明外部变量

void init_fixed_argb2block();
BLOCK_ENUM get_argb2block(unsigned argb, bool isRandom);

#endif /* BLOCK_H */
