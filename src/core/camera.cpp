#include "camera.h"

#include <cmath>

Camera::Camera()
{
    this->_x = glm::normalize(glm::cross(this->_z, this->_y));
    entity.pos = __player_init_pos;
}

Camera::Camera(glm::vec3 pos, glm::vec3 front) : _z(glm::normalize(front))
{
    this->_x = glm::normalize(glm::cross(this->_z, this->_y));
    entity.pos = pos;
}

Camera::Camera(glm::vec3 pos, glm::vec3 lookAt, glm::vec3 front, float fov, float aspect_ratio, float near, float far) : target(lookAt), _z(glm::normalize(front)), fov(fov), aspect_ratio(aspect_ratio), near(near), far(far)
{
    entity.pos = pos;
    this->_x = glm::normalize(glm::cross(this->_z, this->_y));
}

// 停下脚步
void Camera::stay()
{
    entity.v = glm::vec3(0);
}

// 以一定速度平移
void Camera::moveBy(MoveDirection direction, bool walking)
{
    if (walking)
    {
        // 步行模式（平行于xOz面前后移动，不改变y值）
        if (direction == FORWARD)
        {
            glm::vec2 norm = glm::normalize(glm::vec2(_z.x, _z.z));
            this->entity.v.x = norm.x * MOVE_SPEED;
            this->entity.v.z = norm.y * MOVE_SPEED;
        }
        if (direction == BACKWARD)
        {
            glm::vec2 norm = glm::normalize(glm::vec2(_z.x, _z.z));
            this->entity.v.x = -norm.x * MOVE_SPEED;
            this->entity.v.z = -norm.y * MOVE_SPEED;
        }
    }
    else
    {
        // 飞行模式（朝屏幕中心飞行、上升、下降）
        if (direction == FORWARD)
            this->entity.v = this->_z * MOVE_SPEED;
        if (direction == BACKWARD)
            this->entity.v = -this->_z * MOVE_SPEED;
        if (direction == UPWARD)
            this->entity.v = this->_y * MOVE_SPEED;
        if (direction == DOWNWARD)
            this->entity.v = -this->_y * MOVE_SPEED;
    }

    // 步行和飞行通用
    if (direction == LEFT)
        this->entity.v = -this->_x * MOVE_SPEED;
    if (direction == RIGHT)
        this->entity.v = this->_x * MOVE_SPEED;
}

// 跳跃
void Camera::jump()
{
    if (abs(entity.v.y) < 0.1f)
        entity.v.y = 3.0f; // 防止空中连跳
}

// 鼠标控制镜头旋转
void Camera::cursor_rotation(float deltaX, float deltaY)
{
    pitch -= deltaY;
    yaw += deltaX;
    // 俯仰角范围限制
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;
    // 计算镜头朝向方向
    glm::vec3 forward;
    forward.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    forward.y = sin(glm::radians(pitch));
    forward.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));

    _z = glm::normalize(forward);
    _x = glm::normalize(glm::cross(_z, world_up)); // x轴必须在世界坐标系的xOz平面内，否则画面会左右歪斜
    _y = glm::normalize(glm::cross(_x, _z));
}

void Camera::zoom(float delta)
{ // 正放大，负缩小
    fov += delta;
    if (fov > FOV_MAX)
        fov = FOV_MAX;
    if (fov < FOV_MIN)
        fov = FOV_MIN;
}

// 供渲染管线调用，获得VP变换矩阵
void Camera::set_camera_vp_matrices(class GPUCameraData &cameraData)
{
    cameraData.view = glm::lookAt(entity.pos, entity.pos + _z, _y);
    glm::mat4 proj = glm::perspective(glm::radians(fov), aspect_ratio, near, far);
    proj[0][0] *= -1; // 上下颠倒，因为默认读取网格坐标系是上小下大，不符合笛卡尔坐标系
    cameraData.proj = proj;
    cameraData.camPos = glm::vec4(entity.pos, 1);
}

// 获取鼠标指向的位置
glm::vec3 Camera::point_at(bool place_block)
{
    int point_len = 5; // 手长度
    // 从近到远判断是否有方块遮挡
    glm::vec3 fact = entity.pos + _z; // 实际抵达位置，至少比自身远一格
    bool isAir = false;               // 第一次遍历是空气（放置方块用）
    for (int i = 0; i < point_len; ++i)
    {
        int x = (int)fact.x;
        int y = (int)fact.y;
        int z = (int)fact.z;
        if (x < 0 || y < 0 || z < 0)
            return POINT_NOTHING;

        if (!is_air_block(x, y, z))
        {
            if (!place_block)
                return fact; // 破坏这一格
            if (isAir)
                return fact - _z; // 近一格是空气，可以放方块
            return POINT_NOTHING; // 玩家被封住了
        }
        isAir = true;
        fact += _z; // 增长一格
    }
    return POINT_NOTHING;
}
