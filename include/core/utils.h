#ifndef UTILS_H
#define UTILS_H

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

#endif /* UTILS_H */