#include "render_system.h"
#include <glm/gtc/matrix_transform.hpp>

std::vector<RenderObject> __renderables;
std::vector<Chunk *> __rendered_chunks;
std::unordered_map<std::string, Material> __materials;
std::vector<Mesh> __meshes(TEXTURE_SIZE * TEXTURE_SIZE + 1);

Material *RenderSystem::create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string &name)
{
    Material mat;
    mat.pipeline = pipeline;
    mat.pipelineLayout = layout;
    __materials[name] = mat;
    return &__materials[name];
}

// returns nullptr if it can't be found
Material *RenderSystem::get_material(const std::string &name)
{
    auto it = __materials.find(name);
    if (it == __materials.end())
    {
        return nullptr;
    }
    else
    {
        return &(*it).second;
    }
}

// returns nullptr if it can't be found
Mesh *RenderSystem::get_mesh(BLOCK_ENUM block_kind)
{
    int block_id = (int)block_kind;
    if (block_id < __meshes.size())
    {
        return &__meshes[block_id];
    }
    return nullptr;
}

// 初始场景布置【重要】
void RenderSystem::init_scene()
{
    DEFAULT_MATERIAL = get_material("textured");
    init_terrain_heights();
    import_model_resources();
    render_all_chunks(__player_init_pos, false);
}

// 渲染Block的一个面【该方法不带值检查，另外在renderable放置完之后记得赋值face_id】
void RenderSystem::render_face(Block *block, int i)
{
    block->faces[i].material = DEFAULT_MATERIAL;
    glm::mat4 translation = glm::translate(glm::mat4{1.0}, glm::vec3(block->pos) + BLOCK_TRANSLATE_DIST[i]);
    block->faces[i].model_transform = translation * BLOCK_ROTATION[i];
    block->faces[i].normal = FACE_NORMALS[i];
    // 各面不同的渲染
    if (block->kind == BLOCK_GRASS)
    {
        if (i == FACE_U)
        { // U
            block->faces[i].mesh = get_mesh(RENDER_GRASS_TOP);
            return;
        }
        if (i == FACE_D)
        { // D
            block->faces[i].mesh = get_mesh(BLOCK_DIRT);
            return;
        }
        block->faces[i].mesh = get_mesh(BLOCK_GRASS);
        return;
    }
    if (block->kind == BLOCK_TNT)
    {
        if (i == FACE_U)
        { // U
            block->faces[i].mesh = get_mesh(BLOCK_TNT_TOP);
            return;
        }
        if (i == FACE_D)
        { // D
            block->faces[i].mesh = get_mesh(BLOCK_TNT_BOTTOM);
            return;
        }
        block->faces[i].mesh = get_mesh(BLOCK_TNT);
        return;
    }

    block->faces[i].mesh = get_mesh(block->kind);
}

// 取消渲染Block的一个面【该方法不带值检查】
void RenderSystem::unrender_face(Block *block, int i)
{
    int index = block->face_id[i];
    if (index == FACE_UNRENDERED)
        return; // 这个面没有渲染
    block->face_id[i] = FACE_UNRENDERED;
    if (index >= __renderables.size())
        return; // 【在区块交界处调用render_all_blocks()时清空了renderables，进入该分支】
    __renderables[index].mesh = nullptr;
    __renderables[index].material = nullptr;
}

// 更新某个位置方块的面渲染状态
void RenderSystem::render_block(Block *block)
{
    if (block == nullptr || block->kind == BLOCK_AIR)
        return;
    // 【半透明方块】永远渲染全部六个面
    if (block->transparent)
    {
        for (int i = 0; i < 6; ++i)
        {
            render_face(block, i);
            __renderables.push_back(block->faces[i]);
            block->face_id[i] = __renderables.size() - 1;
        }
        return;
    }
    for (int i = 0; i < 6; ++i)
    {                                                    // 遍历 FRUDLB六个面相邻的方块，如果是空气就渲染该面
        glm::ivec3 near_pos = block->pos + BLOCK_DIR[i]; // 邻近该方向的方块位置
        if (near_pos.x >= 0 && near_pos.y >= 0 && near_pos.z >= 0)
        {
            Block *near_block = get_block(near_pos);
            if (!near_block || near_block->kind == BLOCK_AIR)
            { // 没有方块邻近，渲染该面
                render_face(block, i);
                __renderables.push_back(block->faces[i]);
                block->face_id[i] = __renderables.size() - 1;
            }
            else
            {                                     // 清除邻接方块不再暴露空气的面
                unrender_face(near_block, 5 - i); // 邻接面
            }
        }
    }
}

// 移除方块，并更新附近方块的面，【只push_back，导致的一些空内存可以交给区块更新时整理】
void RenderSystem::unrender_block(glm::ivec3 pos)
{
    Block *block = get_block(pos);
    if (block == nullptr || block->kind == BLOCK_AIR)
        return;

    // 将被拆除方块的6个面消除掉
    for (int i = 0; i < 6; ++i)
    {
        unrender_face(block, i);
    }

    // 区块取消渲染不用考虑邻接面
    for (int i = 0; i < 6; ++i)
    {                                             // 遍历 FRUDLB六个面相邻的方块，渲染邻接方块新暴露在空气中的面
        glm::ivec3 near_pos = pos + BLOCK_DIR[i]; // 邻近该方向的方块位置
        if (near_pos.x >= 0 && near_pos.y >= 0 && near_pos.z >= 0)
        {
            Block *near_block = get_block(near_pos);
            if (!near_block || near_block->kind == BLOCK_AIR)
                continue;
            // 如果邻近的方块是透明的，它贴图本身没有被删除，因此不用再渲染
            if (near_block->transparent)
                continue;
            int opp_i = 5 - i;
            render_face(near_block, opp_i);
            __renderables.push_back(near_block->faces[opp_i]);
            near_block->face_id[opp_i] = __renderables.size() - 1;
        }
    }
}

// 渲染该区块中的所有面
void RenderSystem::render_chunk(int rx, int ry, int rz)
{
    if (rx < 0 || ry < 0 || rz < 0)
        return;
    Chunk *chunk = get_chunk(rx, ry, rz);
    if (!chunk)
    {
        chunk = generate_chunk(rx, ry, rz, false);
    }
    if (chunk->rendered)
    {
        return; // 已经渲染过了
    }
    for (int i = 0; i < CHUNK_LEN; ++i)
    {
        for (int j = 0; j < CHUNK_LEN; ++j)
        {
            for (int k = 0; k < CHUNK_LEN; ++k)
            {
                Block *block = chunk->blocks[i][j][k];
                if (!block || block->kind == BLOCK_AIR)
                    continue;
                render_block(block);
            }
        }
    }
    chunk->rendered = true;
    __rendered_chunks.push_back(chunk);
}

/// 取消渲染该区块中的所有面【不删除区块】
void RenderSystem::unrender_chunk(Chunk *chunk)
{
    if (!chunk || !chunk->rendered)
        return;
    chunk->rendered = false;
    for (int i = 0; i < CHUNK_LEN; ++i)
    {
        for (int j = 0; j < CHUNK_LEN; ++j)
        {
            for (int k = 0; k < CHUNK_LEN; ++k)
            {
                Block *block = chunk->blocks[i][j][k];
                if (!block || block->kind == BLOCK_AIR)
                    continue;
                for (int l = 0; l < 6; ++l)
                {
                    unrender_face(block, l);
                }
            }
        }
    }
}

// 重新渲染玩家【附近】所有区块
void RenderSystem::render_all_chunks(glm::vec3 player_pos, bool rerender)
{
    std::vector<RenderObject>().swap(__renderables); // 清空并释放所有空间
    int rx = (int)player_pos.x / CHUNK_LEN;
    int ry = (int)player_pos.y / CHUNK_LEN;
    int rz = (int)player_pos.z / CHUNK_LEN;

    // 先自然生成区块
    if (!rerender)
    {
        for (int i = rx - CHUNK_GEN_RADIUS; i <= rx + CHUNK_GEN_RADIUS; ++i)
        {
            for (int j = ry - CHUNK_GEN_RADIUS; j <= ry + CHUNK_GEN_RADIUS; ++j)
            {
                for (int k = rz - CHUNK_GEN_RADIUS; k <= rz + CHUNK_GEN_RADIUS; ++k)
                {
                    if (glm::length(glm::vec3(i, j, k) - glm::vec3(rx, ry, rz)) <= CHUNK_GEN_RADIUS)
                        generate_chunk(i, j, k, false);
                }
            }
        }
    }
    // 再渲染区块
    for (int i = rx - CHUNK_GEN_RADIUS; i <= rx + CHUNK_GEN_RADIUS; ++i)
    {
        for (int j = ry - CHUNK_GEN_RADIUS; j <= ry + CHUNK_GEN_RADIUS; ++j)
        {
            for (int k = rz - CHUNK_GEN_RADIUS; k <= rz + CHUNK_GEN_RADIUS; ++k)
            {
                if (glm::length(glm::vec3(i, j, k) - glm::vec3(rx, ry, rz)) <= CHUNK_GEN_RADIUS)
                    render_chunk(i, j, k);
            }
        }
    }
}

// 玩家移动时导致区块更新（懒删除离开方向的区块，勤加载前进方向的区块）
void RenderSystem::update_render_chunks(glm::ivec3 chunk_pos, glm::vec3 player_pos)
{
    int chunk_x = chunk_pos.x, chunk_y = chunk_pos.y, chunk_z = chunk_pos.z;
    // 以玩家为中心圆，渲染新区块
    for (int i = chunk_x - CHUNK_RENDER_RADIUS; i <= chunk_x + CHUNK_RENDER_RADIUS; ++i)
    {
        for (int j = chunk_y - CHUNK_RENDER_RADIUS; j <= chunk_y + CHUNK_RENDER_RADIUS; ++j)
        {
            for (int k = chunk_z - CHUNK_RENDER_RADIUS; k <= chunk_z + CHUNK_RENDER_RADIUS; ++k)
            {
                if (glm::length(glm::vec3(i, j, k) - glm::vec3(chunk_pos)) <= CHUNK_RENDER_RADIUS)
                    render_chunk(i, j, k);
            }
        }
    }

    // 面数太多，重载
    if (__renderables.size() > REFRESH_RENDER_FACE_MAX)
    {
        for (int i = 0; i < __rendered_chunks.size(); ++i)
        {
            unrender_chunk(__rendered_chunks[i]);
        }
        //            vector<Chunk*>().swap(__rendered_chunks); // 清空并释放所有空间
        __rendered_chunks.clear(); // 【不应该释放这些空间？】
        render_all_chunks(player_pos, true);
        printf("__renderables overflow, refreshing...");
        return;
    }

    // 懒删除：检查已经渲染的区块数量有多少，超过上限就清理所有超出范围的区块
    if (__rendered_chunks.size() > CHUNK_RENDER_COUNT_MAX)
    {
        printf("too much rendered chunks! %lu\n", __rendered_chunks.size());
        for (int i = 0; i < __rendered_chunks.size(); ++i)
        {
            Chunk *chunk = __rendered_chunks[i];
            if (abs(chunk_x - chunk->cx) > CHUNK_RENDER_RADIUS ||
                abs(chunk_y - chunk->cy) > CHUNK_RENDER_RADIUS ||
                abs(chunk_z - chunk->cz) > CHUNK_RENDER_RADIUS)
            {
                unrender_chunk(chunk);
                __rendered_chunks.erase(__rendered_chunks.begin() + i);
                --i; // 删除前的第i + 1个元素左移到了索引i
            }
        }
        // __rendered_chunks.erase(std::remove(__rendered_chunks.begin(), __rendered_chunks.end(), nullptr), __rendered_chunks.end());
        printf("cleaned far rendered chunks, remain %lu\n", __rendered_chunks.size());
    }
}
