#ifndef NOISE_H
#define NOISE_H

#include <glm/glm.hpp>

using namespace glm;

inline float smoothing(float a, float b, float t)
{
    t = t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
    return mix(a, b, t);
}

inline float random1(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453f);
}

inline vec2 random2(vec2 p)
{
    return fract(sin(vec2(dot(p, vec2(127.1, 311.7)),
                          dot(p, vec2(269.5, 183.3)))) *
                 43758.5453f);
}

inline float noise2D(vec2 n)
{
    return glm::fract(sin(dot(n, glm::vec2(127.1, 311.7))) * 43758.543);
}

inline float interpNoise2D(float x, float y)
{
    int intX = int(floor(x));
    float fractX = glm::fract(x);

    int intY = int(floor(y));
    float fractY = fract(y);

    float v1 = noise2D(vec2(intX, intY));
    float v2 = noise2D(vec2(intX + 1, intY));
    float v3 = noise2D(vec2(intX, intY + 1));
    float v4 = noise2D(vec2(intX + 1, intY + 1));

    float i1 = smoothing(v1, v2, fractX);
    float i2 = smoothing(v3, v4, fractX);

    return smoothing(i1, i2, fractY);
}

inline float fbm(const vec2 uv)
{
    float total = 0;
    float persistence = 0.5f;
    int octaves = 6;
    float freq = 4.f;
    float amp = 0.5f;

    for (int i = 1; i <= octaves; i++)
    {
        total += interpNoise2D(uv.x * freq, uv.y * freq) * amp;

        freq *= 2.f;
        amp *= persistence;
    }
    return total;
}

inline float surflet(vec2 P, vec2 gridPoint)
{
    float distX = abs(P.x - gridPoint.x);
    float distY = abs(P.y - gridPoint.y);
    float tX = 1 - 6 * pow(distX, 5.0) + 15 * pow(distX, 4.0) - 10 * pow(distX, 3.0);
    float tY = 1 - 6 * pow(distY, 5.0) + 15 * pow(distY, 4.0) - 10 * pow(distY, 3.0);

    vec2 gradient = random2(gridPoint);
    vec2 diff = P - gridPoint;
    float height = dot(diff, gradient);
    return height * tX * tY;
}

inline float perlinNoise(vec2 uv)
{
    vec2 uvXLYL = floor(uv);
    vec2 uvXHYL = uvXLYL + vec2(1, 0);
    vec2 uvXHYH = uvXLYL + vec2(1, 1);
    vec2 uvXLYH = uvXLYL + vec2(0, 1);

    return surflet(uv, uvXLYL) + surflet(uv, uvXHYL) + surflet(uv, uvXHYH) + surflet(uv, uvXLYH);
}

inline float worleyNoise(vec2 uv)
{
    vec2 uvInt = floor(uv);
    vec2 uvFract = fract(uv);
    float minDist = 1.0;

    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            vec2 neighbor = vec2(float(x), float(y));

            vec2 point = random2(uvInt + neighbor);

            vec2 diff = neighbor + point - uvFract;
            float dist = length(diff);
            minDist = glm::min(minDist, dist);
        }
    }
    return minDist;
}

inline vec3 random3(vec3 p)
{
    return fract(sin(vec3(dot(p, vec3(127.1, 311.7, 350.7)),
                          dot(p, vec3(269.5, 183.3, 450.6)),
                          dot(p, vec3(420.6, 631.2, 120.1)))) *
                 43758.5453f);
}

inline float surflet3D(vec3 p, vec3 gridPoint)
{
    vec3 t2 = abs(p - gridPoint);
    vec3 t = vec3(0, 0, 0);

    t.x = 1.f - 6.f * pow(t2.x, 5.f) + 15.f * pow(t2.x, 4.f) - 10.f * pow(t2.x, 3.f);
    t.y = 1.f - 6.f * pow(t2.y, 5.f) + 15.f * pow(t2.y, 4.f) - 10.f * pow(t2.y, 3.f);
    t.z = 1.f - 6.f * pow(t2.z, 5.f) + 15.f * pow(t2.z, 4.f) - 10.f * pow(t2.z, 3.f);

    vec3 gradient = random3(gridPoint) * 2.f - vec3(1., 1., 1.);

    vec3 diff = p - gridPoint;

    float height = dot(diff, gradient);

    return height * t.x * t.y * t.z;
}

inline float perlinNoise3D(vec3 p)
{
    float surfletSum = 0.f;

    for (int dx = 0; dx <= 1; ++dx)
    {
        for (int dy = 0; dy <= 1; ++dy)
        {
            for (int dz = 0; dz <= 1; ++dz)
            {
                surfletSum += surflet3D(p, floor(p) + vec3(dx, dy, dz));
            }
        }
    }
    return surfletSum;
}

#endif /* NOISE_H */
