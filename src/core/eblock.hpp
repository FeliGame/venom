#ifndef eblock_hpp
#define eblock_hpp
#include "block.hpp"

// 电子方块
class EBlock : public Block
{
public:
    float potential;      // 电势
    bool is_enable;       // 是否处于使能状态
    bool fixed_potential; // 是否固定电势（电源、接地等是固定的）

    // 构造函数
    EBlock(int p_x, int p_y, int p_z, BLOCK_ENUM p_kind, float p_potential, bool p_is_enable) : Block(p_x, p_y, p_z, p_kind)
    {
        // TODO: 根据p_kind（方块类型）设置是否为固定电势

        potential = p_potential;
        is_enable = p_is_enable;
        fixed_potential = false;
    }
};

#endif /* eblock_hpp */
