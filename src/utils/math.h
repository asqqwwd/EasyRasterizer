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
    typename T::type dot_product_3D(const T &a, const T &b)
    {
        assert(T::size() == 3);
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    }

    template <typename T, typename = typename std::enable_if_t<std::is_arithmetic_v<T>>>
    T saturate(T value)
    {
        return value > 1 ? 1 : (value < 0 ? 0 : value);
    }
}

#endif // ERER_UTILS_MATH_H_