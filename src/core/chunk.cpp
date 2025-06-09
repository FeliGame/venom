#include "chunk.h"
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION

size_t glm_ivec3_hash::operator()(const glm::ivec3 &v) const
{
    // 素数异或哈希
    const size_t p0 = 73856093;
    const size_t p1 = 19349669;
    const size_t p2 = 83492791;
    return (std::hash<int>()(v.x) * p0) ^ (std::hash<int>()(v.y) * p1) ^ (std::hash<int>()(v.z) * p2);
}

std::unordered_map<glm::ivec3, Chunk *, glm_ivec3_hash> __chunks;

Chunk::Chunk(int p_rx, int p_ry, int p_rz) : cx(p_rx), cy(p_ry), cz(p_rz)
{
    // 三维数组memset无法直接初始化
    memset(blocks, 0, sizeof(Block *) * CHUNK_LEN_CUBIC);
}

Chunk::Chunk(int p_rx, int p_ry, int p_rz, bool p_built) : cx(p_rx), cy(p_ry), cz(p_rz), built(p_built)
{
    // 三维数组memset无法直接初始化
    memset(blocks, 0, sizeof(Block *) * CHUNK_LEN_CUBIC);
}


Chunk *get_chunk(int x, int y, int z)
{
    glm::ivec3 pos(x, y, z);
    auto it = __chunks.find(pos);
    if (it != __chunks.end())
    {
        return it->second;
    }
    return nullptr;
}

Chunk *get_or_create_chunk(int x, int y, int z)
{
    Chunk *chunk = get_chunk(x, y, z);
    if (!chunk)
    {
        __chunks.insert({glm::ivec3(x, y, z), chunk = new Chunk(x, y, z)});
    }
    return chunk;
}

// 强制用指定区块进行更新
Chunk *set_chunk(int x, int y, int z, Chunk *chunk)
{
    glm::ivec3 pos(x, y, z);
    auto it = __chunks.find(pos);
    if (it != __chunks.end())
    {
        // 键值对已经存在，更新值
        it->second = chunk;
    }
    else
    {
        // 键值对不存在，插入新的键值对
        __chunks.insert({pos, chunk});
    }
    return chunk;
}

Block *get_block(glm::ivec3 pos)
{
    int x = pos.x;
    int y = pos.y;
    int z = pos.z;
    int cx = x / CHUNK_LEN;
    int cy = y / CHUNK_LEN;
    int cz = z / CHUNK_LEN;
    Chunk *chunk = get_chunk(cx, cy, cz);
    if (!chunk)
        return nullptr; // 没有区块自然也没有方块
    // 在区块内的相对坐标
    int i = x % CHUNK_LEN;
    int j = y % CHUNK_LEN;
    int k = z % CHUNK_LEN;
    return chunk->blocks[i][j][k];
}

//  该方块是不是空气
bool is_air_block(int x, int y, int z)
{
    int cx = x / CHUNK_LEN;
    int cy = y / CHUNK_LEN;
    int cz = z / CHUNK_LEN;
    Chunk *chunk = get_chunk(cx, cy, cz);
    if (!chunk)
        return false; // 没有区块认为是实心的（无法物理穿越也无法互动）
    int i = x % CHUNK_LEN;
    int j = y % CHUNK_LEN;
    int k = z % CHUNK_LEN;
    return chunk->blocks[i][j][k]->kind == BLOCK_AIR;
}

// 创建方块，cover指定是否覆盖原来方块，如果为BLOCK_AIR则必定覆盖，given_chunk已经给了现成的区块，就不用重新再找
Block *create_block(glm::ivec3 pos, BLOCK_ENUM block_kind, bool replace, Chunk *given_chunk)
{
    if (block_kind == BLOCK_NULL || pos.x < 0 || pos.y < 0 || pos.z < 0)
    {
        return nullptr;
    }
    Chunk *chunk;
    if (!given_chunk)
    {
        int cx = pos.x / CHUNK_LEN;
        int cy = pos.y / CHUNK_LEN;
        int cz = pos.z / CHUNK_LEN;
        chunk = get_or_create_chunk(cx, cy, cz);
    }
    else
    {
        chunk = given_chunk;
    }
    // 在区块内的相对坐标
    int i = pos.x % CHUNK_LEN;
    int j = pos.y % CHUNK_LEN;
    int k = pos.z % CHUNK_LEN;
    Block *block = chunk->blocks[i][j][k];
    if (block)
    { // 已经有方块了
        if (!replace && (block->kind != BLOCK_AIR))
            return chunk->blocks[i][j][k];         // 不替换方块时，没有操作，当然空气方块总是可以被替换的
        chunk->blocks[i][j][k]->kind = block_kind; // 如果替换或者是空气块，直接更改原方块数据，避免重新new资源
    }
    chunk->blocks[i][j][k] = block = new Block(pos, block_kind);
    return block;
}
