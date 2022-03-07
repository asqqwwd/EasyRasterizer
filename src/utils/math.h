#ifndef ERER_UTILS_MATH_H_
#define ERER_UTILS_MATH_H_

#include <type_traits> // std::enable_if_v std::is_class_t
#include <random>
#include <vector>
#include <chrono>

#include "../settings.h"

// Shadow map related variables
#define NUM_SAMPLES 20
#define BLOCKER_SEARCH_NUM_SAMPLES NUM_SAMPLES
#define PCF_NUM_SAMPLES NUM_SAMPLES
#define NUM_RINGS 10

namespace Utils
{
    template <typename T>
    T cross_product_3D(const T &a, const T &b)
    {
        assert(T::size() == 3);
        return T{a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
    };

    template <typename T>
    typename T::type dot_product(const T &a, const T &b)
    {
        // assert(T::size() == 3 || T::size() == 2);
        T::type ret = 0;
        for (int i = 0; i < T::size(); ++i)
        {
            ret += a[i] * b[i];
        }
        return ret;
    }

    template <typename T, typename = typename std::enable_if_t<std::is_arithmetic_v<T>>>
    T saturate(T value)
    {
        return value > 1 ? 1 : (value < 0 ? 0 : value);
    }

    template <typename T>
    T lerp(const T &a, const T &b, float value)
    {
        assert(value >= 0 && value <= 1);
        return (1 - value) * a + value * b; // not v*a+(1-v)*b!
    }

    Core::Vector4f tone_mapping(const Core::Vector4c &color)
    {
        return Core::Vector4f{color[0] / 255.f, color[1] / 255.f, color[2] / 255.f, color[3] / 255.f};
    }

    float fract(float x)
    {
        return x - std::floor(x);
    }

    float rand_1to1(float x)
    {
        // -1 -1
        return fract(sin(x) * 10000.0);
    }

    float rand_2to1(Core::Vector2f uv)
    {
        // 0 - 1
        const float a = 12.9898, b = 78.233, c = 43758.5453;
        float dt = dot_product(uv, Core::Vector2f{a, b}), sn = std::fmod(dt, PI);
        return fract(sin(sn) * c);
    }

    std::vector<Core::Vector2f> poissonDiskSamples(const Core::Vector2f &randomSeed)
    {
        float ANGLE_STEP = PI2 * float(NUM_RINGS) / float(NUM_SAMPLES);
        float INV_NUM_SAMPLES = 1.0 / float(NUM_SAMPLES);

        float angle = rand_2to1(randomSeed) * PI2;
        float radius = INV_NUM_SAMPLES;
        float radiusStep = radius;

        std::vector<Core::Vector2f> ret;
        for (int i = 0; i < NUM_SAMPLES; i++)
        {
            ret.push_back(Core::Vector2f{cos(angle), sin(angle)} * std::pow(radius, 0.75));
            radius += radiusStep;
            angle += ANGLE_STEP;
        }
        return ret;
    }

    std::vector<Core::Vector2f> uniformDiskSamples(const Core::Vector2f &randomSeed)
    {
        float randNum = rand_2to1(randomSeed);
        float sampleX = rand_1to1(randNum);
        float sampleY = rand_1to1(sampleX);

        float angle = sampleX * PI2;
        float radius = std::sqrt(sampleY);

        std::vector<Core::Vector2f> ret;
        for (int i = 0; i < NUM_SAMPLES; i++)
        {
            ret.push_back(Core::Vector2f{cos(angle), sin(angle)} * std::pow(radius, 0.75));

            sampleX = rand_1to1(sampleY);
            sampleY = rand_1to1(sampleX);

            angle = sampleX * PI2;
            radius = std::sqrt(sampleY);
        }
        return ret;
    }

}

#endif // ERER_UTILS_MATH_H_