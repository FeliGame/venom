// 区块操作

#ifndef CHUNK_H
#define CHUNK_H

// #define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <block.h>
#include <noise.h>
#include <spline.h>

// 读取纹理png需要使用
#include <stb_image.h>

static const int CHUNK_LEN = 8;                            // 区块边长
static const int CHUNK_LEN_SQUARE = CHUNK_LEN * CHUNK_LEN; // 区块边长的平方
static const int CHUNK_LEN_CUBIC = CHUNK_LEN * CHUNK_LEN * CHUNK_LEN;
// 使用较小的数组保证随机访问效率，超出该范围就用从磁盘交换【区域】技术解决
static const int CHUNK_MAX_XZ = 64; // x, z轴区块数量
static const int CHUNK_MAX_Y = 32;  // y轴区块数量

static const bool RANDOM_BUILDING_TEXTURE = true; // 生成建筑的纹理贴图是否随机
static float MODEL_MAGNIFICATION = 2.5f;          // 建筑相对导入模型坐标系的放大比例

static const double TICK_PERIOD = 0.2; // 时间刻长度（秒）

#include <glm/glm.hpp>
#include <unordered_map>

class Chunk;

struct glm_ivec3_hash
{
    size_t operator()(const glm::ivec3 &v) const;
};

extern std::unordered_map<glm::ivec3, Chunk *, glm_ivec3_hash> __chunks;

class Chunk
{
private:
    Chunk() {}

public:
    Block *blocks[CHUNK_LEN][CHUNK_LEN][CHUNK_LEN]; // 其中的所有方块，按x->y->z顺序
    int cx, cy, cz;                                 // 区块位置
    bool rendered = false;                          // 该区块是否已经被渲染（避免重复渲染）
    bool built = false;                             // 该区块是否有建筑盘踞（一个区块最多只能有一所建筑）

    Chunk(int p_rx, int p_ry, int p_rz);
    Chunk(int p_rx, int p_ry, int p_rz, bool p_built);
};


Chunk *get_chunk(int x, int y, int z);

Chunk *get_or_create_chunk(int x, int y, int z);

// 强制用指定区块进行更新
Chunk *set_chunk(int x, int y, int z, Chunk *chunk);

Block *get_block(glm::ivec3 pos);

//  该方块是不是空气
bool is_air_block(int x, int y, int z);

// 创建方块，cover指定是否覆盖原来方块，如果为BLOCK_AIR则必定覆盖，given_chunk已经给了现成的区块，就不用重新再找
Block *create_block(glm::ivec3 pos, BLOCK_ENUM block_kind, bool replace, Chunk *given_chunk);

#endif /* CHUNK_H */
