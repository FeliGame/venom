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

    Block(int p_x, int p_y, int p_z, BLOCK_ENUM p_kind) : pos(glm::ivec3(p_x, p_y, p_z)), kind(p_kind)
    {
        memset(face_id, FACE_UNRENDERED, sizeof(face_id));
        if (kind == BLOCK_SPIDER_WEB || kind == BLOCK_ROSE || kind == BLOCK_YELLOW_FLOWER)
            transparent = true;
        else
            transparent = false;
    }
    Block(glm::ivec3 p_pos, BLOCK_ENUM p_kind) : pos(p_pos), kind(p_kind)
    {
        memset(face_id, FACE_UNRENDERED, sizeof(face_id));
        if (kind == BLOCK_SPIDER_WEB || kind == BLOCK_ROSE || kind == BLOCK_YELLOW_FLOWER)
            transparent = true;
        else
            transparent = false;
    }
};

// 【模型导入】颜色->Block种类映射关系

static std::unordered_map<unsigned, BLOCK_ENUM> __argb2block; // 颜色到Block种类的映射关系，除了下面两个函数不要直接访问该map！！！

static inline void init_fixed_argb2block()
{

    // 导入参考下列例子，十六进制从左往右依次是ARGB
    //    __argb2block[0xFF949494] = BLOCK_CLAY;
    //    __argb2block[0xFF7E7E7E] = BLOCK_COBBLE_STONE;
    //    __argb2block[0xFF865C42] = BLOCK_DIRT;
}

// isRandom随机一一映射，根据材质颜色随机分配rgb
static inline BLOCK_ENUM get_argb2block(unsigned argb, bool isRandom)
{
    if (isRandom && __argb2block.find(argb) == __argb2block.end())
    {
        // 颜色键不存在值，插入一个随机值
        __argb2block[argb] = static_cast<BLOCK_ENUM>(rand() % 128); // 随机范围对应方块枚举最大值
    }
    return __argb2block[argb];
}

#endif /* BLOCK_H */
