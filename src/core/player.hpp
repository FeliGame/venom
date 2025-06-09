#ifndef player_hpp
#define player_hpp

#include <glm/glm.hpp>

static glm::vec3 __player_init_pos = {5.f, 60.f, 5.f};

class Player
{
public:
    BLOCK_ENUM hold_block = BLOCK_SPIDER_WEB; // 玩家持有的方块种类
};

static Player __player;

#endif /* player_hpp */
