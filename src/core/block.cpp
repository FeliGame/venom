#include "block.h"
#include <cstring>
#include <cstdlib>

std::unordered_map<unsigned, BLOCK_ENUM> __argb2block; // 定义外部变量

Block::Block(int p_x, int p_y, int p_z, BLOCK_ENUM p_kind) : pos(glm::ivec3(p_x, p_y, p_z)), kind(p_kind)
{
    std::memset(face_id, FACE_UNRENDERED, sizeof(face_id));
    if (kind == BLOCK_SPIDER_WEB || kind == BLOCK_ROSE || kind == BLOCK_YELLOW_FLOWER)
        transparent = true;
    else
        transparent = false;
    brightness = LIGHT_SCALE;
}

Block::Block(glm::ivec3 p_pos, BLOCK_ENUM p_kind) : pos(p_pos), kind(p_kind)
{
    std::memset(face_id, FACE_UNRENDERED, sizeof(face_id));
    if (kind == BLOCK_SPIDER_WEB || kind == BLOCK_ROSE || kind == BLOCK_YELLOW_FLOWER)
        transparent = true;
    else
        transparent = false;
    brightness = LIGHT_SCALE;
}

void init_fixed_argb2block()
{
    // 导入参考下列例子，十六进制从左往右依次是ARGB
    //    __argb2block[0xFF949494] = BLOCK_CLAY;
    //    __argb2block[0xFF7E7E7E] = BLOCK_COBBLE_STONE;
    //    __argb2block[0xFF865C42] = BLOCK_DIRT;
}

// isRandom随机一一映射，根据材质颜色随机分配rgb
BLOCK_ENUM get_argb2block(unsigned argb, bool isRandom)
{
    if (isRandom && __argb2block.find(argb) == __argb2block.end())
    {
        // 颜色键不存在值，插入一个随机值
        __argb2block[argb] = static_cast<BLOCK_ENUM>(rand() % 128); // 随机范围对应方块枚举最大值
    }
    return __argb2block[argb];
}