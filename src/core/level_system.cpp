#include "level_system.h"

void LevelSystem::update_sky_color()
{ // 更新天空
    auto now = std::chrono::high_resolution_clock::now();
    float past_tick = (std::chrono::duration_cast<std::chrono::microseconds>(now - __clock_game_start).count()) * TICK_PERIOD;

    float w = 0.000005 * past_tick + PI; // 越小时间流速越慢
    sky_color.x = (std::sin(w) + 1) / 2;
    sky_color.y = (1 - std::sin(w)) / 2 + 0.15;
    sky_color.z = (1 - std::sin(w)) / 2 + 0.05; // 和R变化相反

    // 考虑太阳光方向计算各颜色波长吸收率
}
