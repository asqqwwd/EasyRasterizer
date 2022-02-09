#include "tgaimage.h"
#include <iostream>
#include <fstream>
#include <cstring>

namespace Utils
{
    TGAImage::TGAImage() : data_(), width_(0), height_(0), bytespp_(0) {}
    TGAImage::TGAImage(const int w, const int h, const int bpp) : data_(w * h * bpp, 0), width_(w), height_(h), bytespp_(bpp) {}

    bool TGAImage::read_tga_file(const std::string filename)
    {
        std::ifstream in;
        in.open(filename, std::ios::binary);
        if (!in.is_open())
        {
            std::cerr << "can't open file " << filename << "\n";
            in.close();
            return false;
        }
        TGA_Header header;
        in.read(reinterpret_cast<char *>(&header), sizeof(header));
        if (!in.good())
        {
            in.close();
            std::cerr << "an error occured while reading the header\n";
            return false;
        }
        width_ = header.width;
        height_ = header.height;
        bytespp_ = header.bitsperpixel >> 3;
        if (width_ <= 0 || height_ <= 0 || (bytespp_ != GRAYSCALE && bytespp_ != RGB && bytespp_ != RGBA))
        {
            in.close();
            std::cerr << "bad bpp (or width/height) value\n";
            return false;
        }
        size_t nbytes = bytespp_ * width_ * height_;
        data_ = std::vector<std::uint8_t>(nbytes, 0);
        if (3 == header.datatypecode || 2 == header.datatypecode)
        {
            in.read(reinterpret_cast<char *>(data_.data()), nbytes);
            if (!in.good())
            {
                in.close();
                std::cerr << "an error occured while reading the data\n";
                return false;
            }
        }
        else if (10 == header.datatypecode || 11 == header.datatypecode)
        {
            if (!load_rle_data(in))
            {
                in.close();
                std::cerr << "an error occured while reading the data\n";
                return false;
            }
        }
        else
        {
            in.close();
            std::cerr << "unknown file format " << (int)header.datatypecode << "\n";
            return false;
        }
        if (!(header.imagedescriptor & 0x20))
            flip_vertically();
        if (header.imagedescriptor & 0x10)
            flip_horizontally();
        std::cerr << width_ << "x" << height_ << "/" << bytespp_ * 8 << "\n";
        in.close();
        return true;
    }

    bool TGAImage::load_rle_data(std::ifstream &in)
    {
        size_t pixelcount = width_ * height_;
        size_t currentpixel = 0;
        size_t currentbyte = 0;
        TGAColor colorbuffer;
        do
        {
            std::uint8_t chunkheader = 0;
            chunkheader = in.get();
            if (!in.good())
            {
                std::cerr << "an error occured while reading the data\n";
                return false;
            }
            if (chunkheader < 128)
            {
                chunkheader++;
                for (int i = 0; i < chunkheader; i++)
                {
                    in.read(reinterpret_cast<char *>(colorbuffer.bgra), bytespp_);
                    if (!in.good())
                    {
                        std::cerr << "an error occured while reading the header\n";
                        return false;
                    }
                    for (int t = 0; t < bytespp_; t++)
                        data_[currentbyte++] = colorbuffer.bgra[t];
                    currentpixel++;
                    if (currentpixel > pixelcount)
                    {
                        std::cerr << "Too many pixels read\n";
                        return false;
                    }
                }
            }
            else
            {
                chunkheader -= 127;
                in.read(reinterpret_cast<char *>(colorbuffer.bgra), bytespp_);
                if (!in.good())
                {
                    std::cerr << "an error occured while reading the header\n";
                    return false;
                }
                for (int i = 0; i < chunkheader; i++)
                {
                    for (int t = 0; t < bytespp_; t++)
                        data_[currentbyte++] = colorbuffer.bgra[t];
                    currentpixel++;
                    if (currentpixel > pixelcount)
                    {
                        std::cerr << "Too many pixels read\n";
                        return false;
                    }
                }
            }
        } while (currentpixel < pixelcount);
        return true;
    }

    bool TGAImage::write_tga_file(const std::string filename, const bool vflip, const bool rle) const
    {
        std::uint8_t developer_area_ref[4] = {0, 0, 0, 0};
        std::uint8_t extension_area_ref[4] = {0, 0, 0, 0};
        std::uint8_t footer[18] = {'T', 'R', 'U', 'E', 'V', 'I', 'S', 'I', 'O', 'N', '-', 'X', 'F', 'I', 'L', 'E', '.', '\0'};
        std::ofstream out;
        out.open(filename, std::ios::binary);
        if (!out.is_open())
        {
            std::cerr << "can't open file " << filename << "\n";
            out.close();
            return false;
        }
        TGA_Header header;
        header.bitsperpixel = bytespp_ << 3;
        header.width = width_;
        header.height = height_;
        header.datatypecode = (bytespp_ == GRAYSCALE ? (rle ? 11 : 3) : (rle ? 10 : 2));
        header.imagedescriptor = vflip ? 0x00 : 0x20; // top-left or bottom-left origin
        out.write(reinterpret_cast<const char *>(&header), sizeof(header));
        if (!out.good())
        {
            out.close();
            std::cerr << "can't dump the tga file\n";
            return false;
        }
        if (!rle)
        {
            out.write(reinterpret_cast<const char *>(data_.data()), width_ * height_ * bytespp_);
            if (!out.good())
            {
                std::cerr << "can't unload raw data\n";
                out.close();
                return false;
            }
        }
        else
        {
            if (!unload_rle_data(out))
            {
                out.close();
                std::cerr << "can't unload rle data\n";
                return false;
            }
        }
        out.write(reinterpret_cast<const char *>(developer_area_ref), sizeof(developer_area_ref));
        if (!out.good())
        {
            std::cerr << "can't dump the tga file\n";
            out.close();
            return false;
        }
        out.write(reinterpret_cast<const char *>(extension_area_ref), sizeof(extension_area_ref));
        if (!out.good())
        {
            std::cerr << "can't dump the tga file\n";
            out.close();
            return false;
        }
        out.write(reinterpret_cast<const char *>(footer), sizeof(footer));
        if (!out.good())
        {
            std::cerr << "can't dump the tga file\n";
            out.close();
            return false;
        }
        out.close();
        return true;
    }

    // TODO: it is not necessary to break a raw chunk for two equal pixels (for the matter of the resulting size)
    bool TGAImage::unload_rle_data(std::ofstream &out) const
    {
        const std::uint8_t max_chunk_length = 128;
        size_t npixels = width_ * height_;
        size_t curpix = 0;
        while (curpix < npixels)
        {
            size_t chunkstart = curpix * bytespp_;
            size_t curbyte = curpix * bytespp_;
            std::uint8_t run_length = 1;
            bool raw = true;
            while (curpix + run_length < npixels && run_length < max_chunk_length)
            {
                bool succ_eq = true;
                for (int t = 0; succ_eq && t < bytespp_; t++)
                    succ_eq = (data_[curbyte + t] == data_[curbyte + t + bytespp_]);
                curbyte += bytespp_;
                if (1 == run_length)
                    raw = !succ_eq;
                if (raw && succ_eq)
                {
                    run_length--;
                    break;
                }
                if (!raw && !succ_eq)
                    break;
                run_length++;
            }
            curpix += run_length;
            out.put(raw ? run_length - 1 : run_length + 127);
            if (!out.good())
            {
                std::cerr << "can't dump the tga file\n";
                return false;
            }
            out.write(reinterpret_cast<const char *>(data_.data() + chunkstart), (raw ? run_length * bytespp_ : bytespp_));
            if (!out.good())
            {
                std::cerr << "can't dump the tga file\n";
                return false;
            }
        }
        return true;
    }

    TGAColor TGAImage::get(const int x, const int y) const
    {
        if (!data_.size() || x < 0 || y < 0 || x >= width_ || y >= height_)
            return {};
        return TGAColor(data_.data() + (x + y * width_) * bytespp_, bytespp_);
    }

    void TGAImage::set(int x, int y, const TGAColor &c)
    {
        if (!data_.size() || x < 0 || y < 0 || x >= width_ || y >= height_)
            return;
        memcpy(data_.data() + (x + y * width_) * bytespp_, c.bgra, bytespp_);
    }

    int TGAImage::get_bytespp()
    {
        return bytespp_;
    }

    int TGAImage::get_width() const
    {
        return width_;
    }

    int TGAImage::get_height() const
    {
        return height_;
    }

    void TGAImage::flip_horizontally()
    {
        if (!data_.size())
            return;
        int half = width_ >> 1;
        for (int i = 0; i < half; i++)
        {
            for (int j = 0; j < height_; j++)
            {
                TGAColor c1 = get(i, j);
                TGAColor c2 = get(width_ - 1 - i, j);
                set(i, j, c2);
                set(width_ - 1 - i, j, c1);
            }
        }
    }

    void TGAImage::flip_vertically()
    {
        if (!data_.size())
            return;
        size_t bytes_per_line = width_ * bytespp_;
        std::vector<std::uint8_t> line(bytes_per_line, 0);
        int half = height_ >> 1;
        for (int j = 0; j < half; j++)
        {
            size_t l1 = j * bytes_per_line;
            size_t l2 = (height_ - 1 - j) * bytes_per_line;
            std::copy(data_.begin() + l1, data_.begin() + l1 + bytes_per_line, line.begin());
            std::copy(data_.begin() + l2, data_.begin() + l2 + bytes_per_line, data_.begin() + l1);
            std::copy(line.begin(), line.end(), data_.begin() + l2);
        }
    }

    std::uint8_t *TGAImage::buffer()
    {
        return data_.data();
    }

    void TGAImage::clear()
    {
        data_ = std::vector<std::uint8_t>(width_ * height_ * bytespp_, 0);
    }

    void TGAImage::scale(int w, int h)
    {
        if (w <= 0 || h <= 0 || !data_.size())
            return;
        std::vector<std::uint8_t> tdata(w * h * bytespp_, 0);
        int nscanline = 0;
        int oscanline = 0;
        int erry = 0;
        size_t nlinebytes = w * bytespp_;
        size_t olinebytes = width_ * bytespp_;
        for (int j = 0; j < height_; j++)
        {
            int errx = width_ - w;
            int nx = -bytespp_;
            int ox = -bytespp_;
            for (int i = 0; i < width_; i++)
            {
                ox += bytespp_;
                errx += w;
                while (errx >= (int)width_)
                {
                    errx -= width_;
                    nx += bytespp_;
                    memcpy(tdata.data() + nscanline + nx, data_.data() + oscanline + ox, bytespp_);
                }
            }
            erry += h;
            oscanline += olinebytes;
            while (erry >= (int)height_)
            {
                if (erry >= (int)height_ << 1) // it means we jump over a scanline
                    memcpy(tdata.data() + nscanline + nlinebytes, tdata.data() + nscanline, nlinebytes);
                erry -= height_;
                nscanline += nlinebytes;
            }
        }
        data_ = tdata;
        width_ = w;
        height_ = h;
    }

    void TGAImage::write_data(const int w, const int h, const int bpp, uint8_t *data)
    {
        data_.clear();
        data_.shrink_to_fit();
        data_ = std::vector<uint8_t>(w * h * bpp, 0);
        width_ = w;
        height_ = h;
        bytespp_ = bpp;
        memcpy(data_.data(), data, w * h * bpp);
    }

}