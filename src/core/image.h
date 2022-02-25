#ifndef ERER_CORE_IMAGE_H_
#define ERER_CORE_IMAGE_H_

#include <cstdint> // for uint*_t
#include <assert.h>
namespace Core
{
    template <typename T>
    class Image
    {
    private:
        T *data_ = nullptr;
        int width_ = 0;
        int height_ = 0;

    public:
        Image()
        {
        }
        Image(int w, int h) : data_(new T[w * h]()), width_(w), height_(h)
        {
            assert(w > 0 && h > 0);
        }
        Image(T *data, int w, int h) : width_(w), height_(h)
        {
            assert(w > 0 && h > 0);
            data_ = data;
            data = nullptr;
        }
        ~Image()
        {
            if (data_ != nullptr)
            {
                delete[] data_;
            }
        }

        /* Copy semantics */
        Image(const Image &other)
        {
            width_ = other.width_;
            height_ = other.height_;
            data_ = new T[width_ * height_]();
            std::copy(other.data_, other.data_ + width_ * height_, data_);
        }
        Image &operator=(const Image &other)
        {
            if (this != &other)
            {
                width_ = other.width_;
                height_ = other.height_;
                delete[] data_;
                data_ = new T[width_ * height_];
                std::copy(other.data_, other.data_ + width_ * height_, data_);
            }
            return *this;
        }

        /* Move semantics */
        Image(Image &&other)
        {
            width_ = other.width_;
            height_ = other.height_;
            data_ = other.data_;   // you can directly access other's private variables here
            other.data_ = nullptr; // assign the data members of the source object to the default value, which prevents the destructor from repeatedly releasing the resource
        }
        Image &operator=(Image &&other)
        {
            if (this != &other)
            {
                width_ = other.width_;
                height_ = other.height_;
                delete[] data_; // for dynamic arrays created in the heap, you need to delete them manually
                data_ = other.data_;
                other.data_ = nullptr;
            }
            return *this;
        }

        void memset(const T &value)
        {
            assert(width_ > 0 && height_ > 0);
            for (int i = width_ * height_; i--; data_[i] = value)
                ;
        }
        T get(int x, int y) const
        {
            // assert(data_ != nullptr && x >= 0 && y >= 0 && x < width_ && y < height_);
            return data_[y * width_ + x];
        }
        void set(int x, int y, const T &value)
        {
            // assert(data_ != nullptr && x >= 0 && y >= 0 && x < width_ && y < height_);
            data_[y * width_ + x] = value;
        }
        int get_width() const
        {
            return width_;
        }
        int get_height() const
        {
            return height_;
        }
        T sampling(float u, float v) const
        {
            // (..,0)&(1,..)->[0,1),any value that not in [0,1] cannot reach 1 after clip
            float clip_u = u > 1 ? u - std::floor(u) : (u < 0 ? -(std::floor(u) - u) : u);
            float clip_v = v > 1 ? v - std::floor(v) : (v < 0 ? -(std::floor(v) - v) : v);
            int x = static_cast<int>(std::round(clip_u * (width_ - 1)));
            int y = static_cast<int>(std::round(clip_v * (height_ - 1)));
            return data_[y * width_ + x];
        }
        T sampling(int x, int y) const
        {
            return data_[y * width_ + x];
        }
    };
}
#endif // ERER_CORE_IMAGE_H_
