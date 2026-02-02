#pragma once

#include <vector>
#include <array>
#include <cmath>
#include <type_traits>
#include <iostream>
#include "tiny_vector.hpp"

// Wrap index into [0, n)
inline int wrap(int i, int n)
{
    i %= n;
    return (i < 0) ? i + n : i;
}



namespace dtks{


    


    template<class T>
    class Image2d{
        public:


        Image2d() = default;
        
        Image2d(std::array<int, 2> shape) : shape_(shape)
        {
            data_.resize(shape_[0] * shape_[1]);
        }

        Image2d(std::array<int, 2> shape, T initial_value) : shape_(shape)
        {
            data_.resize(shape_[0] * shape_[1], initial_value);
        }


        template<class U>
        Image2d(const Image2d<U> & other) : shape_(other.shape())
        {
            data_.resize(shape_[0] * shape_[1]);
            for(std::size_t i = 0; i < data_.size(); ++i)
            {
                data_[i] = static_cast<T>(other[i]);
            }
        }

        T& operator()(int x, int y)
        {
            return data_[y * shape_[0] + x];
        }
        const T& operator()(int x, int y) const
        {
            return data_[y * shape_[0] + x];
        }

        T& operator[](std::size_t index)
        {
            return data_[index];
        }

        const T& operator[](std::size_t index) const
        {
            return data_[index];
        }

        template<class U>
        T& operator[](const std::array<U, 2>& coord)
        {
            return data_[coord[1] * shape_[0] + coord[0]];
        }

        template<class U>
        const T& operator[](const std::array<U, 2>& coord) const
        {
            return data_[coord[1] * shape_[0] + coord[0]];
        }

        const std::array<int, 2>& shape() const
        {
            return shape_;
        }
        std::size_t size() const
        {
            return data_.size();
        }

        auto data()
        {
            return data_.data();
        }


        private:
        std::array<int, 2> shape_;
        std::vector<T> data_;
    };


    // element wise operators (we focus on those we need)
    #define DEFINE_ELEMENTWISE_OP(OP) \
    template<class T, class U> \
    Image2d<T>& operator OP( \
        Image2d<T>& img, \
        const U & element \
    ) \
    { \
        for(std::size_t i = 0; i < img.size(); ++i) \
        { \
            img[i] OP element; \
        } \
        return img; \
    }

    DEFINE_ELEMENTWISE_OP(+=)
    DEFINE_ELEMENTWISE_OP(-=)
    DEFINE_ELEMENTWISE_OP(*=)
    DEFINE_ELEMENTWISE_OP(/=)

    #undef DEFINE_ELEMENTWISE_OP


















    template<class T, std::size_t N>
    using MultiChannelImage2d = Image2d<TinyVector<T, N>>;


    template<class T, std::size_t N>
    std::pair<
        std::array<T, N>,
        std::array<T, N>
    > channel_min_max(const MultiChannelImage2d<T, N> & image)
    {
        std::array<T, N> min_vals;
        std::array<T, N> max_vals;

        for(std::size_t c = 0; c < N; ++c)
        {
            min_vals[c] = std::numeric_limits<T>::max();
            max_vals[c] = std::numeric_limits<T>::lowest();
        }

        for(std::size_t i = 0; i < image.size(); ++i)
        {
            const auto & pixel = image[i];
            for(std::size_t c = 0; c < N; ++c)
            {
                if(pixel[c] < min_vals[c]) min_vals[c] = pixel[c];
                if(pixel[c] > max_vals[c]) max_vals[c] = pixel[c];
            }
        }

        return {min_vals, max_vals};
    }


    template<class T>
    struct zero
    {
        static T value()
        {
            return T(0);
        }
    };

    template<class T, std::size_t N>
    struct zero<TinyVector<T, N>>
    {
        static TinyVector<T, N> value()
        {
            TinyVector<T, N> result;
            for(std::size_t i = 0; i < N; ++i)
            {
                result[i] = T(0);
            }
            return result;
        }
    };


    
    template<class T, class SCALAR>
    void add_weighted(T & target, const T & value, SCALAR weight)
    {
        target += value * weight;
    }
    template<class T, class SCALAR, std::size_t N>
    void add_weighted(std::array<T, N> & target, const std::array<T, N> & value, SCALAR weight)
    {
        for(std::size_t i = 0; i < N; ++i)
        {
            target[i] += value[i] * weight;
        }
    }

    template<typename T,class U>
    void gaussianSeparableWrap(
        const Image2d<T>& src_image,
        Image2d<T>& tmp,
        Image2d<U>& dst_image,
        std::size_t kernelR,
        double sigma
    )
    {
        const int r = static_cast<int>(kernelR);
        const int ksize = 2 * r + 1;
            
        const auto width = src_image.shape()[0];
        const auto height = src_image.shape()[1];

        using K = double;

        // --- build 1D Gaussian kernel on stack ---
        K kernel[64]; // assume ksize <= 64 (adjust if needed)
        K sum = K(0);

        for (int i = -r; i <= r; ++i)
        {
            K v = std::exp(-(i * i) / (K(2) * sigma * sigma));
            kernel[i + r] = v;
            sum += v;
        }

        // normalize
        for (int i = 0; i < ksize; ++i)
        {
            kernel[i] /= sum;
        }

        // --- horizontal pass ---
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                T acc = zero<T>::value();

                for (int i = -r; i <= r; ++i)
                {
                    int ix = wrap(x + i, width);
                    add_weighted(acc, src_image(ix, y), kernel[i + r]);
                }

                tmp[y * width + x] = acc;
            }
        }

        // --- vertical pass ---
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                T acc = zero<T>::value();

                for (int i = -r; i <= r; ++i)
                {
                    int iy = wrap(y + i, height);
                    add_weighted(acc, tmp[iy * width + x], kernel[i + r]);
                }

                dst_image[y * width + x] = static_cast<U>(acc);
            }
        }
    }




    template<typename T>
    void gaussianSeparableWrap(
        const Image2d<T>& src_image,
        Image2d<T>& dst_image,
        std::size_t kernelR,
        double sigma
    )
    {
        Image2d<T> tmp(src_image);
        gaussianSeparableWrap(
            src_image,
            tmp,
            dst_image,
            kernelR,
            sigma
        );
    }
    

    template<typename T, typename U, class COMPERATOR>
    void discMorphImpl(
        const Image2d<T>& src_image,
        Image2d<T>& temp_image,
        Image2d<U>& dst_image,
        int radius,
        COMPERATOR comparator
    )
    {
        const int r = radius;
        const int diameter = 2 * r + 1;
            
        const auto width = src_image.shape()[0];
        const auto height = src_image.shape()[1];



        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                T value = src_image(x, y);

                for (int dy = -r; dy <= r; ++dy)
                {
                    for (int dx = -r; dx <= r; ++dx)
                    {
                        if(dx*dx + dy*dy <= r*r)
                        {
                            int ix = x + dx;
                            int iy = y + dy;
                            if(ix >=0 && ix < width && iy >=0 && iy < height)
                            {

                                if(comparator(src_image(ix, iy), value))
                                {
                                    value = src_image(ix, iy);
                                }
                            }
                        }
                    }
                }
                temp_image(x,y) = value;
            }
        }
        for(auto i=0; i<dst_image.size(); ++i)
        {
            dst_image[i] = static_cast<U>(temp_image[i]);
        }
    }
    
    template<typename T, typename U>
    void discErosion(
        const Image2d<T>& src_image,
        Image2d<U>& dst_image,
        int radius
    ){
        Image2d<T> temp_image(src_image.shape());
        discMorphImpl(
            src_image,
            temp_image,
            dst_image,
            radius,
            std::less<T>()
        );
    }

    template<typename T, typename U>
    void discDilation(
        const Image2d<T>& src_image,
        Image2d<U>& dst_image,
        int radius
    ){
        Image2d<T> temp_image(src_image.shape());
        discMorphImpl(
            src_image,
            temp_image,
            dst_image,
            radius,
            std::greater<T>()
        );
    }

    template<typename T, typename U>
    void discOpening(
        const Image2d<T>& src_image,
        Image2d<U>& dst_image,
        int radius
    ){
        Image2d<T> temp_image1(src_image.shape());
        Image2d<T> temp_image2(src_image.shape());
        discMorphImpl(
            src_image,
            temp_image1,
            temp_image2,
            radius,
            std::less<T>()
        );
        discMorphImpl(
            temp_image2,
            temp_image1,
            dst_image,
            radius,
            std::greater<T>()
        );
    }

    template<typename T, typename U>
    void discClosing(
        const Image2d<T>& src_image,
        Image2d<U>& dst_image,
        int radius
    ){      
        Image2d<T> temp_image1(src_image.shape());
        Image2d<T> temp_image2(src_image.shape());
        discMorphImpl(
            src_image,
            temp_image1,
            temp_image2,
            radius,
            std::greater<T>()
        );
        discMorphImpl(
            temp_image2,
            temp_image1,
            dst_image,
            radius,
            std::less<T>()
        );
    }




} // namespace ant