#ifndef ERER_UTILS_MATH_H_
#define ERER_UTILS_MATH_H_

namespace Utils
{
    template <typename T>
    T cross_product_3D(const T &a, const T &b)
    {
        return T{a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
    };
}

#endif // ERER_UTILS_MATH_H_