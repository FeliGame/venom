// 执行物理仿真的每个实体
#ifndef PHYSICS_ENTITY_H
#define PHYSICS_ENTITY_H

#include <glm/gtx/transform.hpp>

struct PhysicsEntity
{
    glm::vec3 pos;   // 位置
    glm::vec3 v;     // 速度
    glm::vec3 force; // 所受合外力
    float m = 1.0f;  // 质量
    // 【四棱柱，且与空间坐标系始终平行】
    int h = 1; // 高度
    int d = 1; // 直径
};

#endif /* PHYSICS_ENTITY_H */
