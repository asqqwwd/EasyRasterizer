#ifndef ERER_UTILS_MATH_H_
#define ERER_UTILS_MATH_H_

#include <type_traits> // std::enable_if_v std::is_class_t
#include <random>
#include <vector>
#include <chrono>

#include "../settings.h"

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

    namespace Random
    {
        std::default_random_engine generator;
        std::uniform_real_distribution<float> distribution;
        float get_random_float_01()
        {
            generator.seed(static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count()));
            return distribution(generator);
        }
    }

    namespace RandomSample
    {
        float fract(float x)
        {
            return x - std::floor(x);
        }

        float rand_1to1(float x)
        {
            // -1 -1
            return fract(std::sin(x) * 10000.0f);
        }

        float rand_2to1(Core::Vector2f uv)
        {
            // 0 - 1
            const float a = 12.9898f, b = 78.233f, c = 43758.5453f;
            float dt = dot_product(uv, Core::Vector2f{a, b}), sn = std::fmod(dt, static_cast<float>(PI));
            return fract(sin(sn) * c);
        }

        std::vector<Core::Vector2f> poissonDiskSamples(int num_rings = 10, int num_samples = 50)
        {
            Core::Vector2f randomSeed{Random::get_random_float_01(), Random::get_random_float_01()};

            float ANGLE_STEP = static_cast<float>(PI2 * num_rings / num_samples);
            float INV_NUM_SAMPLES = static_cast<float>(1.f / num_samples);

            float angle = rand_2to1(randomSeed) * static_cast<float>(PI2);  // random initial angle
            float radius = INV_NUM_SAMPLES;
            float radiusStep = radius;

            std::vector<Core::Vector2f> ret;
            for (int i = 0; i < num_samples; i++)
            {
                ret.push_back(Core::Vector2f{std::cos(angle), std::sin(angle)} * std::pow(radius, 0.75f));
                radius += radiusStep;
                angle += ANGLE_STEP;
            }
            return ret;
        }

        std::vector<Core::Vector2f> uniformDiskSamples(int num_samples = 20)
        {
            Core::Vector2f randomSeed{Random::get_random_float_01(), Random::get_random_float_01()};

            float randNum = rand_2to1(randomSeed);
            float sampleX = rand_1to1(randNum);
            float sampleY = rand_1to1(sampleX);

            float angle = sampleX * static_cast<float>(PI2);
            float radius = std::sqrt(sampleY);

            std::vector<Core::Vector2f> ret;
            for (int i = 0; i < num_samples; i++)
            {
                ret.push_back(Core::Vector2f{std::cos(angle), std::sin(angle)} * std::pow(radius, 0.75f));

                sampleX = rand_1to1(sampleY);
                sampleY = rand_1to1(sampleX);

                angle = sampleX * static_cast<float>(PI2);
                radius = std::sqrt(sampleY);
            }
            return ret;
        }
    }

}

#endif // ERER_UTILS_MATH_H_