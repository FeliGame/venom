#ifndef PHYSICS_SYSTEM_H
#define PHYSICS_SYSTEM_H

#include <camera.h>
#include <cmath>
#include <render_system.h>

static bool PLAYER_PHYSICS = true; // 玩家的物理碰撞、重力

class PhysicsSystem
{
private:
    const glm::vec3 g = {.0f, -0.5f, .0f};

public:
    void simulate(float dt)
    {
        glm::vec3 player_last_pos = __main_camera.entity.pos;
        glm::ivec3 last_chunk = glm::ivec3(player_last_pos) / CHUNK_LEN;

        // 对每个物理实体进行计算，半隐式积分法
        if (PLAYER_PHYSICS)
            __main_camera.entity.v += g * dt;
        __main_camera.entity.pos += __main_camera.entity.v * dt;

        PhysicsEntity player = __main_camera.entity;
        int x = (int)player.pos.x;
        int y = (int)player.pos.y;
        int z = (int)player.pos.z;
        int d = player.d;
        int h = player.h;

        if (PLAYER_PHYSICS)
        { // 玩家和方块碰撞判定
            int xz_size = CHUNK_MAX_XZ * CHUNK_LEN;
            int y_size = CHUNK_MAX_Y * CHUNK_LEN;
            // 界内限制（根据下面的__block_map的定义域反推）
            x = std::min(xz_size - d, std::max(d, x));
            y = std::min(y_size - 1, std::max(h, y));
            z = std::min(xz_size - d, std::max(d, z));

            // 方块碰撞
            if ((player.v.x > 0 && !is_air_block(x + d, y, z)) ||
                (player.v.x < 0 && !is_air_block(x - d, y, z)))
            {
                __main_camera.entity.v.x = 0;
                __main_camera.entity.pos.x = player_last_pos.x;
            }
            if ((player.v.y > 0 && !is_air_block(x, y + 1, z)) || // 头顶
                (player.v.y < 0 && !is_air_block(x, y - h, z)))
            {
                __main_camera.entity.v.y = 0;
                __main_camera.entity.pos.y = player_last_pos.y;
            }
            if ((player.v.z > 0 && !is_air_block(x, y, z + d)) ||
                (player.v.z < 0 && !is_air_block(x, y, z - d)))
            {
                __main_camera.entity.v.z = 0;
                __main_camera.entity.pos.z = player_last_pos.z;
            }
        }

        // 边界条件，防止陷入地面
        if (player.pos.y <= 0)
        {
            __main_camera.entity.v.y = 0;
            __main_camera.entity.pos.y = 0;
        }

        glm::ivec3 chunk = glm::ivec3(player.pos) / CHUNK_LEN;
        // 玩家之前所在区块发生改变，就重新渲染该区块
        if (last_chunk != chunk)
        {
            __render_system.update_render_chunks(chunk, player.pos);
        }
    }
};

static PhysicsSystem __physics_system;

#endif /* PHYSICS_SYSTEM_H */
