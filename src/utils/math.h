#ifndef ERER_UTILS_MATH_H_
#define ERER_UTILS_MATH_H_

#include <type_traits> // std::enable_if_v std::is_class_t

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
        return (1 - value) * a + value * b;  // not v*a+(1-v)*b!
    }

    Core::Vector4f tone_mapping(const Core::Vector4c &color)
    {
        return Core::Vector4f{color[0] / 255.f, color[1] / 255.f, color[2] / 255.f, color[3] / 255.f};
    }
}

#endif // ERER_UTILS_MATH_H_