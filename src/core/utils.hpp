#ifndef utils_hpp
#define utils_hpp

#include <glm/glm.hpp>

// 相邻六个方向
static const glm::ivec3 ADJACENT_IVEC3[] = {
    {-1, 0, 0},
    {1, 0, 0}, 
    {0, -1, 0},
    {0, 1, 0}, 
    {0, 0, -1},
    {0, 0, 1}  
};

static const glm::ivec3 ADJACENT_VEC3[] = {
    {-1, 0, 0},
    {1, 0, 0}, 
    {0, -1, 0},
    {0, 1, 0}, 
    {0, 0, -1},
    {0, 0, 1}  
};

#endif /* utils_hpp */