// 控制相机的各种行为

#ifndef CAMERA_H
#define CAMERA_H

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <window_system.h>
#include <physics_entity.h>
#include <level.h>
#include <player.h>
#include <iostream>

static uint32_t MAX_WIDTH = 800;
static uint32_t MAX_HEIGHT = 600;
static const float CAMERA_NEAREST = 0.1f;
static const float CAMERA_FURTHEST = 5000.0f;
static const float FOV_MAX = 36000.0f;
static const float FOV_MIN = 10.0f;

static const float MOVE_SPEED = .3f; // 每帧移动距离

static const glm::vec3 POINT_NOTHING = glm::vec3(-1); // 鼠标没有指到什么物体上

enum MoveDirection
{
    FORWARD,
    BACKWARD,
    RIGHT,
    LEFT,
    UPWARD,
    DOWNWARD
};

class Camera
{
private:
public:
    glm::vec3 target = {50.f, 10.f, 10.f}; // 朝向坐标
    glm::vec3 _y = {.0f, 1.f, .0f};        // 相机上方指向
    glm::vec3 _x = {1.f, .0f, .0f};
    glm::vec3 _z = {.0f, .0f, 1.f};       // 相机前方指向
    glm::vec3 world_up = {.0f, 1.f, .0f}; // 世界的上方指向，恒定

    float fov = 6400.0f;
    float aspect_ratio = MAX_WIDTH / MAX_HEIGHT;
    float near = CAMERA_NEAREST;
    float far = CAMERA_FURTHEST;

    // 欧拉角（角度制）
    float pitch = .0f;    // 俯仰角
    float yaw = 90.0f;    // 偏航角
    PhysicsEntity entity; // 物理实体，包含了相机的实际位置、速度、质量等

    Camera();
    Camera(glm::vec3 pos, glm::vec3 front);
    Camera(glm::vec3 pos, glm::vec3 lookAt, glm::vec3 front, float fov, float aspect_ratio, float near, float far);

    // 停下脚步
    void stay();

    // 以一定速度平移
    void moveBy(MoveDirection direction, bool walking);

    // 跳跃
    void jump();

    // 鼠标控制镜头旋转
    void cursor_rotation(float deltaX, float deltaY);

    void zoom(float delta);

    // 供渲染管线调用，获得VP变换矩阵
    void set_camera_vp_matrices(class GPUCameraData &cameraData);

    // 获取鼠标指向的位置
    glm::vec3 point_at(bool place_block);
};

static Camera __main_camera; // 主相机

#endif /* CAMERA_H */
