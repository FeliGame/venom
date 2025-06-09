// 各种样条曲线，定义域和值域为[-1, 1]

#ifndef spline_hpp
#define spline_hpp

namespace spline
{
    // 给定参数，生成四次多项式
    static inline float poly4(float x, float a, float b, float c, float d, float f)
    {
        float x_2 = x * x;
        float x_3 = x_2 * x;
        float x_4 = x_2 * x_2;
        return a * x_4 + b * x_3 + c * x_2 + d * x + f;
    }

    // 大陆性（急缓急）
    static inline float continent(float x)
    {
        return poly4(x, 2.41, -2.376, -1.62, 2.33, 0.177);
    }

    // 侵蚀性
    static inline float erosion(float x)
    {
        return poly4(x, -1.72, -1.465, 2.682, 0.454, -0.957);
    }

    // 山脊/山谷性
    static inline float peak_valley(float x)
    {
        return poly4(x, -1.585, -0.164, 1.924, 1.05, -0.422);
    }

};
#endif /* spline_hpp */
