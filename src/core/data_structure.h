#ifndef ERER_CORE_DATA_STRUCTURE_H_
#define ERER_CORE_DATA_STRUCTURE_H_

#include <iostream>
#include <cmath>
#include <cassert>

template <typename... Args>
class ArgsExtractor
{
}; // 泛化模板，没有任何调用和它匹配

template <typename T, typename U, typename... Args>
class ArgsExtractor<T *, U, Args...> : public ArgsExtractor<T *, Args...>
{
public:
    ArgsExtractor(T *value_p, U value, Args... args) : ArgsExtractor<T *, Args...>(++value_p, args...) // 这里不能用value_p++，因为value_p++为右值，无法被修改
    {
        --value_p; // 恢复指针
        value_ = *value_p = static_cast<U>(value);
    }

    const U &get_value() const
    {
        return value_;
    } // const函数不能返回可变引用，只能返回const引用或者copy版本
    const ArgsExtractor<T *, Args...> &get_next() const
    {
        return *this; // 返回值隐式转换为子类引用
    }

private:
    U value_;
}; // 偏特化模板，前N-1条调用与此匹配

template <typename T>
class ArgsExtractor<T *>
{
public:
    ArgsExtractor(T *value_p)
    {
        // --value_p;  // 这里进行指针移动，没有任何效果？
    } // 若最后args为空后仍带参数，偏特化模板中的public构造函数的必要的！
};    // 偏特化模板，最后一条调用和此匹配

template <typename T, int N, typename... Args>
class Vector
{
public:
    // 构造/析构函数
    Vector(Args... args)
    {
        ArgsExtractor<T *, Args...> args_extractor(data_, args...); // 将args...中的数据提取到data_中
    }                                                               // 构造函数
    ~Vector() = default;                                                      // 析构函数

    // Vector(const Vector<N> &);               // 拷贝构造函数
    // Vector<N> &operator=(const Vector<N> &); // 拷贝赋值运算符

    // Vector(Vector<N> &&) = delete;               // 移动构造函数
    // Vector<N> &operator=(Vector<N> &&) = delete; // 移动赋值运算符

    // // 转换构造函数，即允许哪些类型转换为该类
    // Vector(float);

    // // 自定义转换函数，即允许该类转化为其他哪些类型(标准类 or 非标准类)
    // operator float();

    // 作为成员函数重载运算符，= -> [] ()不能重载为友元函数
    T &operator[](int i)
    {
        assert(i >= 0 && i < N);
        return data_[i];
    } // 可被当成左值对返回元素进行修改
    T operator[](int i) const
    {
        assert(i >= 0 && i < N);
        return data_[i];
    } // 实例化后的const Vector会调用此函数

    // 作为友元函数重载运算符，可以重载特殊计算顺序
    template <typename T, int N, typename... Args>  // 这个必须得加，否则会报错：无法解析的外部符号
    friend Vector<T, N, Args...> operator+(const Vector<T, N, Args...> &, const Vector<T, N, Args...> &);
    // friend Vector<N> operator+(const Vector<N> &, float);
    // friend Vector<N> operator+(float, const Vector<N> &);
    friend int test(int,int);

    // friend std::ostream &operator<<(std::ostream &out, const Vector<N> &v);
    // friend Vector<N1> embed(const Vector<N2> &v, float fill = 1);
    // friend Vector<N1> proj(const Vector<N2> &v);

private:
    T data_[N];
};

template <typename T, int N, typename... Args>
Vector<T, N, Args...> operator+(const Vector<T, N, Args...> &lhs, const Vector<T, N, Args...> &rhs)
{
    Vector<T, N, Args...> ret = lhs;
    for (int i = N; i--; ret[i] += rhs[i])
        ;
    return ret;
}

typedef Vector<float, 3, float, float, float> Vector3f;

#endif  // ERER_CORE_DATA_STRUCTURE_H_