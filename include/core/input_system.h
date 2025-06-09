#ifndef INPUT_SYSTEM_H
#define INPUT_SYSTEM_H

#include <window_system.h>

#include <GLFW/glfw3.h> //负责前端窗口管理
#include <GLFW/glfw3native.h>
#include <iostream>
#include <camera.h>
#include <render_system.h>

using namespace std;

static const float CURSOR_SENSITIVITY = .05f; // 鼠标灵敏度

static bool GLFW_PRESSING[349]; // 按键是否处于按下状态

// 键盘按下瞬间
static inline bool isKeyDown(int key)
{
    if (glfwGetKey(__window, key) == GLFW_PRESS && !GLFW_PRESSING[key])
    {
        GLFW_PRESSING[key] = true;
        return true;
    }
    return false;
}

// 键盘按下保持不动
static inline bool isKeyPressing(int key)
{
    return glfwGetKey(__window, key) == GLFW_PRESS;
}

// 键盘松开瞬间
static inline bool isKeyUp(int key)
{
    if (glfwGetKey(__window, key) == GLFW_RELEASE && GLFW_PRESSING[key])
    {
        GLFW_PRESSING[key] = false;
        return true;
    }
    return false;
}

// 鼠标按下瞬间
static inline bool isMouseButtonDown(int button)
{
    if (glfwGetMouseButton(__window, button) == GLFW_PRESS && !GLFW_PRESSING[button])
    {
        GLFW_PRESSING[button] = true;
        return true;
    }
    return false;
}

// 鼠标按下保持不动
static inline bool isMouseButtonPressing(int button)
{
    return glfwGetMouseButton(__window, button) == GLFW_PRESS;
}

// 鼠标松开瞬间
static inline bool isMouseButtonUp(int button)
{
    if (glfwGetMouseButton(__window, button) == GLFW_RELEASE && GLFW_PRESSING[button])
    {
        GLFW_PRESSING[button] = false;
        return true;
    }
    return false;
}

static bool entering_cursor = false; // Tab刚锁定窗口时，防止视角跳变
static void cursor_callback(GLFWwindow *window, double posX, double posY)
{
    static double lastX, lastY;
    if (entering_cursor)
    {
        entering_cursor = false;
        lastX = posX;
        lastY = posY;
    }
    double deltaX = posX - lastX, deltaY = posY - lastY;

    lastX = posX;
    lastY = posY;

    __main_camera.cursor_rotation(deltaX * CURSOR_SENSITIVITY, deltaY * CURSOR_SENSITIVITY);
    //    __light_camera.cursor_rotation(deltaX * CURSOR_SENSITIVITY, deltaY * CURSOR_SENSITIVITY);
}

static void scroll_callback(GLFWwindow *window, double offsetX, double offsetY)
{
    __main_camera.zoom(offsetY);
}

// 检测输入事件
static void get_input()
{
    // 将鼠标指针锁定在窗口内
    static bool cursor_locked;
    glfwSetInputMode(__window, GLFW_CURSOR, !cursor_locked ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

    if (isKeyDown(GLFW_KEY_TAB))
    {
        cursor_locked = !cursor_locked;
        if (cursor_locked)
            entering_cursor = true;
    }
    else if (isKeyUp(GLFW_KEY_TAB))
    {
    }
    else if (isKeyPressing(GLFW_KEY_TAB))
    {
    }
    // 捕获鼠标指针信息
    if (cursor_locked)
    {
        glfwSetCursorPosCallback(__window, cursor_callback);
        glfwSetScrollCallback(__window, scroll_callback);
    }
    else
    {
        glfwSetCursorPosCallback(__window, nullptr);
        glfwSetScrollCallback(__window, nullptr);
    }

    if (isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT))
    {
    }
    else if (isMouseButtonUp(GLFW_MOUSE_BUTTON_LEFT))
    {
    }
    else if (isMouseButtonPressing(GLFW_MOUSE_BUTTON_LEFT))
    {
        glm::vec3 pick_pos = __main_camera.point_at(false);
        if (pick_pos != POINT_NOTHING)
        {
            __render_system.unrender_block(glm::ivec3(pick_pos));
            Chunk::create_block(glm::ivec3(pick_pos), BLOCK_AIR, true, nullptr); // 清理原方块内存数据
        }
    }

    if (isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
    {
        glm::vec3 pick_pos = __main_camera.point_at(true);
        if (pick_pos != POINT_NOTHING)
        {
            Block *block = Chunk::create_block(glm::ivec3(pick_pos), __player.hold_block, false, nullptr);
            __render_system.render_block(block);
        }
    }
    else if (isMouseButtonUp(GLFW_MOUSE_BUTTON_RIGHT))
    {
    }
    else if (isMouseButtonPressing(GLFW_MOUSE_BUTTON_RIGHT))
    {
    }

    if (isKeyDown(GLFW_KEY_SPACE))
    {
        __main_camera.jump();
    }
    else if (isKeyUp(GLFW_KEY_SPACE))
    {
    }
    else if (isKeyPressing(GLFW_KEY_SPACE))
    {
    }

    if (isKeyDown(GLFW_KEY_A))
    {
    }
    else if (isKeyUp(GLFW_KEY_A))
    {
        __main_camera.stay();
    }
    else if (isKeyPressing(GLFW_KEY_A))
    {
        __main_camera.moveBy(LEFT, PLAYER_PHYSICS);
    }

    if (isKeyDown(GLFW_KEY_D))
    {
    }
    else if (isKeyUp(GLFW_KEY_D))
    {
        __main_camera.stay();
    }
    else if (isKeyPressing(GLFW_KEY_D))
    {
        __main_camera.moveBy(RIGHT, PLAYER_PHYSICS);
    }

    if (isKeyDown(GLFW_KEY_W))
    {
    }
    else if (isKeyUp(GLFW_KEY_W))
    {
        __main_camera.stay();
    }
    else if (isKeyPressing(GLFW_KEY_W))
    {
        __main_camera.moveBy(FORWARD, PLAYER_PHYSICS);
    }

    if (isKeyDown(GLFW_KEY_S))
    {
    }
    else if (isKeyUp(GLFW_KEY_S))
    {
        __main_camera.stay();
    }
    else if (isKeyPressing(GLFW_KEY_S))
    {
        __main_camera.moveBy(BACKWARD, PLAYER_PHYSICS);
    }

    if (isKeyDown(GLFW_KEY_G))
    { // 切换玩家飞行模式
        PLAYER_PHYSICS = !PLAYER_PHYSICS;
    }
    else if (isKeyUp(GLFW_KEY_G))
    {
    }
    else if (isKeyPressing(GLFW_KEY_G))
    {
    }

    // 调试用，非正常游戏功能！
    if (isKeyDown(GLFW_KEY_UP))
    {
    }
    else if (isKeyUp(GLFW_KEY_UP))
    {
    }
    else if (isKeyPressing(GLFW_KEY_UP))
    {
    }

    if (isKeyDown(GLFW_KEY_DOWN))
    {
    }
    else if (isKeyUp(GLFW_KEY_DOWN))
    {
    }
    else if (isKeyPressing(GLFW_KEY_DOWN))
    {
    }

    if (isKeyDown(GLFW_KEY_LEFT))
    {
    }
    else if (isKeyUp(GLFW_KEY_LEFT))
    {
    }
    else if (isKeyPressing(GLFW_KEY_LEFT))
    {
    }

    if (isKeyDown(GLFW_KEY_RIGHT))
    {
    }
    else if (isKeyUp(GLFW_KEY_RIGHT))
    {
    }
    else if (isKeyPressing(GLFW_KEY_RIGHT))
    {
    }
}

#endif /* INPUT_SYSTEM_H */
