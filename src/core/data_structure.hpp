#ifndef ERER_CORE_DATA_STRUCTURE_H_
#define ERER_CORE_DATA_STRUCTURE_H_

#include <iostream>
#include <cassert>          // assert static_assert
#include <typeinfo>         // typeid
#include <initializer_list> //std::initializer_list
#include <type_traits>      // std::enable_if_v std::is_class_t
#include <cmath>

namespace Core
{
    /* Generalized tensor template class declaration and definition */
    template <typename T, int M>
    class Tensor
    {
    private:
        T *data_ = nullptr;

    public:
        /* Constructor and Destructor */
        Tensor() : data_(new T[M]()) // dynamic array padding with zeros
        {
        }
        Tensor(const std::initializer_list<T> &il) : data_(new T[M]())
        {
            for (int i = M; i--;) // prohibition of using i--
            {
                if (std::is_arithmetic_v<T>)
                {
                    data_[i] = static_cast<T>(*(il.begin() + i));
                }
                else
                {
                    data_[i] = *(il.begin() + i); // call copy=
                }
            }
        }
        Tensor(Tensor *other_p) : data_(nullptr)
        {
            data_ = new T[M]();
            std::copy(other_p->data_, other_p->data_ + M, data_);
        } // basically equivalent to the move constructor
        ~Tensor()
        {
            if (data_ != nullptr)
            {
                delete[] data_;
            }
        }

        /* Copy semantics */
        Tensor(const Tensor &other) noexcept : data_(new T[M])
        {
            std::copy(other.data_, other.data_ + M, data_);
        }
        Tensor &operator=(const Tensor &other) noexcept
        {
            if (this != &other)
            {
                delete[] data_;
                data_ = new T[M];
                std::copy(other.data_, other.data_ + M, data_);
            }
            return *this;
        }

        /* Move semantics */
        Tensor(Tensor &&other) noexcept : data_(nullptr)
        {
            data_ = other.data_;   // you can directly access other's private variables here
            other.data_ = nullptr; // assign the data members of the source object to the default value, which prevents the destructor from repeatedly releasing the resource
        }
        Tensor &operator=(Tensor &&other) noexcept
        {
            if (this != &other)
            {
                delete[] data_; // for dynamic arrays created in the heap, you need to delete them manually
                data_ = other.data_;
                other.data_ = nullptr;
            }
            return *this;
        }

        /* Convert semantics */
        // convert constructor (i.e., what types are allowed to be converted to this class)
        Tensor(float value)
        {
            if (data_ == nullptr)
            {
                data_ = new T[M]();
            }
            for (int i = M; i--; data_[i] = std::is_class<T>::value ? value : static_cast<T>(value))
                ;
        } // implicit convert is tolarable
        // Custom convert function (i.e., what other types (standard or non-standard) this class is allowed to convert to)
        // operator std::vector<T>()
        // {
        //     std::vector<T> ret;
        //     for (int i = 0; i < M; i++)
        //     {
        //         ret.push_back(data_[i]);
        //     }
        //     return ret;
        // } // implicit convert is tolarable (e.g., Vector3f a;vector<float> b = a)

        /* Overloading operators as member functions, = -> [] () is exceptional */
        inline T &operator[](int i)
        {
            assert(i >= 0 && i < M);
            return data_[i];
        } // the returned element can be modified as an lvalue
        inline T operator[](int i) const
        {
            assert(i >= 0 && i < M);
            return data_[i];
        } // the instantiated const vector will call this function
        inline Tensor &operator+=(const Tensor &vec)
        {
            for (int i = M; i--; data_[i] += vec[i])
                ;
            return *this; // move()
        }                 // return type cannot be the reference of the local variable  because the local variable will be destroyed after calling, but return of the copy of the local variable will not be destroyed
        inline Tensor &operator+=(float value)
        {
            for (int i = M; i--; data_[i] += value)
                ;
            return *this;
        }
        inline Tensor operator+(const Tensor &vec) const
        {
            return Tensor(*this) += vec;
        }
        inline Tensor operator+(float value) const
        {
            return Tensor(*this) += value;
        }
        inline Tensor &operator-=(const Tensor &vec)
        {
            for (int i = M; i--; data_[i] -= vec[i])
                ;
            return *this;
        }
        inline Tensor &operator-=(float value)
        {
            for (int i = M; i--; data_[i] -= value)
                ;
            return *this;
        }
        inline const Tensor operator-(const Tensor &vec) const
        {
            return Tensor(*this) -= vec;
        }
        inline const Tensor operator-(float value) const
        {
            return Tensor(*this) -= value;
        }
        inline Tensor &operator*=(const Tensor &vec)
        {
            for (int i = M; i--; data_[i] *= vec[i])
                ;
            return *this;
        }
        inline Tensor &operator*=(float value)
        {
            for (int i = M; i--; data_[i] *= value)
                ;
            return *this;
        }
        inline Tensor operator*(const Tensor &vec) const
        {
            return Tensor(*this) *= vec;
        }
        inline Tensor operator*(float value) const
        {
            return Tensor(*this) *= value;
        }

        inline Tensor &operator/=(const Tensor &vec)
        {
            try
            {
                for (int i = M; i--; data_[i] /= vec[i])
                    ;
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }
            return *this;
        }
        inline Tensor &operator/=(float value)
        {
            try
            {
                for (int i = M; i--; data_[i] = static_cast<T>(data_[i] / value))
                    ;
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }
            return *this;
        }
        inline Tensor operator/(const Tensor &vec) const
        {
            return Tensor(*this) /= vec;
        }
        inline Tensor operator/(float value) const
        {
            return Tensor(*this) /= value;
        }

        /* Overloading opearator as a friend function, you can overload a special calculation order */
        template <typename, int> // friend function type parameters can't be same with class type parameters
        friend Tensor operator+(float, const Tensor &);
        template <typename, int>
        friend Tensor operator-(float, const Tensor &);
        template <typename, int>
        friend Tensor operator*(float, const Tensor &);
        template <typename, int>
        friend Tensor operator/(float, const Tensor &);
        template <typename, int>
        friend std::ostream &operator<<(std::ostream &, const Tensor &);

        template <int _NEWROW, typename U = T, typename = typename std::enable_if_t<std::is_arithmetic_v<U>>>
        Tensor<U, _NEWROW> reshape(float fill = 0) const
        {
            Tensor<U, _NEWROW> ret;
            for (int i = _NEWROW; i--; ret[i] = (i < M ? data_[i] : fill))
                ;
            return ret;
        }

        template <int _NEWROW, int _NEWCOL, typename U = T, typename = typename std::enable_if_t<std::is_class_v<U>>>
        Tensor<Tensor<typename U::type, _NEWCOL>, _NEWROW> reshape(float fill = 0) const
        {
            Tensor<Tensor<typename U::type, _NEWCOL>, _NEWROW> ret(fill);
            for (int i = std::min(M, _NEWROW); i--;)
            {
                for (int j = std::min(U::size(), _NEWCOL); j--;)
                {
                    ret[i][j] = data_[i][j];
                }
            }
            return ret;
        }

        template <typename U = T, typename = typename std::enable_if_t<std::is_class_v<U>>> // can't directly use T
        Tensor<Tensor<typename U::type, M>, U::size()> transpose() const
        {
            Tensor<Tensor<typename U::type, M>, U::size()> ret;
            for (int i = U::size(); i--;)
            {
                for (int j = M; j--;)
                {
                    ret[i][j] = data_[j][i];
                }
            }
            return ret;
        }

        template <typename U = T, typename = typename std::enable_if_t<std::is_arithmetic_v<U>>>
        Tensor<U, M> transpose() const
        {
            return *this;
        }

        template <typename U, int N>
        Tensor<U, M> mul(const Tensor<U, N> &vec) const
        {
            static_assert(T::size() == N, "Input dim must be [M N]*[N L]");

            Tensor<U, M> ret;
            auto transposed = vec.transpose();
            for (int i = M; i--;)
            {
                ret[i] = data_[i].mul(transposed);
            }
            return ret;
        }

        template <int L>
        Tensor<T, L> mul(const Tensor<Tensor<T, M>, L> &vecs) const
        {
            Tensor<T, L> ret;
            for (int i = L; i--;)
            {
                ret[i] = (*this).mul(vecs[i]);
            }
            return ret;
        } // for multiple multiplication

        template <typename U = T, typename = typename std::enable_if_t<std::is_arithmetic_v<U>>>
        U mul(const Tensor &vec) const // Tensor<U,M> mislead compiler to infer U by param, then default template param will be discard that cause every tensor class to have this function
        {
            U ret = 0;
            for (int i = M; i--;)
            {
                ret += data_[i] * vec[i];
            }
            return ret;
        }

        template <typename U = T, typename = typename std::enable_if_t<std::is_arithmetic_v<U>>>
        Tensor normal() const
        {
            U square_sum = 0;
            for (int i = M; i--;)
            {
                square_sum += data_[i] * data_[i];
            }
            return Tensor(*this) / l2norm();
        }

        template <typename U = T, typename = typename std::enable_if_t<std::is_arithmetic_v<U>>>
        U l2norm() const
        {
            U square_sum = 0;
            for (int i = M; i--;)
            {
                square_sum += data_[i] * data_[i];
            }
            return std::sqrt(square_sum);
        }

        static constexpr int size()
        {
            return M;
        } // you must indicate constexpr for passing compiling, and static is aimed at getting static variable
        using type = T;
    };

    template <typename T, int M>
    Tensor<T, M> operator+(float value, const Tensor<T, M> &vec)
    {
        return Tensor<T, M>(vec) += value;
    }
    template <typename T, int M>
    Tensor<T, M> operator-(float value, const Tensor<T, M> &vec)
    {
        return Tensor<T, M>(vec) -= value;
    }
    template <typename T, int M>
    Tensor<T, M> operator*(float value, const Tensor<T, M> &vec)
    {
        return Tensor<T, M>(vec) *= value;
    }
    template <typename T, int M>
    Tensor<T, M> operator/(float value, const Tensor<T, M> &vec)
    {
        Tensor<T, M> ret = vec;
        try
        {
            for (int i = M; i--; ret[i] = static_cast<T>(value / vec[i]))
                ;
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
        return ret;
    }
    template <typename T, int M>
    std::ostream &operator<<(std::ostream &out, const Tensor<T, M> &vec)
    {
        char separator = '\0';
        if (std::is_arithmetic<T>::value)
        {
            separator = ' ';
        }
        for (int i = 0; i < M; i++)
        {
            out << vec[i] << separator;
        }
        out << std::endl;
        return out;
    }

    template <typename T, int M>
    Tensor<Tensor<T, M>, M> indentity()
    {
        Tensor<Tensor<T, M>, M> ret;
        for (int i = M; i--;)
        {
            ret[i][i] = 1;
        }
        return ret;
    }

    /* Alias template declaration */
    using Vector2f = Tensor<float, 2>;
    using Vector3f = Tensor<float, 3>;
    using Vector4f = Tensor<float, 4>;
    using Vector2i = Tensor<int, 2>;
    using Vector3i = Tensor<int, 3>;
    using Vector4i = Tensor<int, 4>;
    using Vector2c = Tensor<uint8_t, 2>;
    using Vector3c = Tensor<uint8_t, 3>;
    using Vector4c = Tensor<uint8_t, 4>;

    using Matrix3i = Tensor<Tensor<int, 3>, 3>;
    using Matrix3f = Tensor<Tensor<float, 3>, 3>;
    using Matrix4f = Tensor<Tensor<float, 4>, 4>;
}

#endif // ERER_CORE_DATA_STRUCTURE_H_