#pragma once

#include <array>
#include <cmath>



namespace ant
{

    template <typename T, std::size_t N>
    class TinyVector
    {
        public:

        constexpr TinyVector() = default;
        constexpr TinyVector(const std::array<T, N>& arr) : data_(arr) {}

        // initialize from T
        constexpr TinyVector(const T& value)
        {
            for (std::size_t i = 0; i < N; ++i) {
                data_[i] = value;
            }
        }
        // initialize from initializer list
        constexpr TinyVector(const std::initializer_list<T>& list)
        {
            std::size_t i = 0;
            for (const auto& value : list) {
                if (i < N) {
                    data_[i++] = value;
                } else {
                    break;
                }
            }
        }

        // element access
        constexpr T& operator[](std::size_t i) { return data_[i]; }
        constexpr const T& operator[](std::size_t i) const { return data_[i]; }

        // iterators (so it behaves like a container)
        constexpr auto begin() { return data_.begin(); }
        constexpr auto end()   { return data_.end(); }
        constexpr auto begin() const { return data_.begin(); }
        constexpr auto end()   const { return data_.end(); }
 




        private:
            std::array<T, N> data_{};
    };



    // binary operators + - * / with another TinyVector
    #define DEFINE_BINARY_OPERATOR(OP) \
        template <typename T, typename U, std::size_t N> \
        TinyVector<decltype(std::declval<T>() OP std::declval<U>()), N> operator OP( \
            const TinyVector<T, N>& a, \
            const TinyVector<U, N>& b \
        ) \
        { \
            TinyVector<decltype(std::declval<T>() OP std::declval<U>()), N> result; \
            for (std::size_t i = 0; i < N; ++i) { \
                result[i] = a[i] OP b[i]; \
            } \
            return result; \
        }

    DEFINE_BINARY_OPERATOR(+)
    DEFINE_BINARY_OPERATOR(-)
    DEFINE_BINARY_OPERATOR(*)
    DEFINE_BINARY_OPERATOR(/)
    #undef DEFINE_BINARY_OPERATOR

    //  + - * / with scalar
    #define DEFINE_SCALAR_OPERATOR(OP) \
        template <typename T, typename U, std::size_t N> \
        TinyVector<decltype(std::declval<T>() OP std::declval<U>()), N> operator OP( \
            const TinyVector<T, N>& vec, const U& scalar) \
        { \
            TinyVector<decltype(std::declval<T>() OP std::declval<U>()), N> result; \
            for (std::size_t i = 0; i < N; ++i) { \
                result[i] = vec[i] OP scalar; \
            } \
            return result; \
        } \
        template <typename T, typename U, std::size_t N> \
        TinyVector<decltype(std::declval<U>() OP std::declval<T>()), N> operator OP( \
            const U& scalar, const TinyVector<T, N>& vec) \
        { \
            TinyVector<decltype(std::declval<U>() OP std::declval<T>()), N> result; \
            for (std::size_t i = 0; i < N; ++i) { \
                result[i] = scalar OP vec[i]; \
            } \
            return result; \
        }   

    DEFINE_SCALAR_OPERATOR(+)
    DEFINE_SCALAR_OPERATOR(-)
    DEFINE_SCALAR_OPERATOR(*)
    DEFINE_SCALAR_OPERATOR(/)
    #undef DEFINE_SCALAR_OPERATOR


    // += , -=, *=, /= with another TinyVector
    #define DEFINE_BINARY_OPERATOR_ASSIGNMENT(OP) \
        template <typename T, typename U, std::size_t N> \
        TinyVector<T, N>& operator OP(TinyVector<T, N>& a, const TinyVector<U, N>& b) \
        { \
            for (std::size_t i = 0; i < N; ++i) { \
                a[i] OP b[i]; \
            } \
            return a; \
        }

    DEFINE_BINARY_OPERATOR_ASSIGNMENT(+=)
    DEFINE_BINARY_OPERATOR_ASSIGNMENT(-=)
    DEFINE_BINARY_OPERATOR_ASSIGNMENT(*=)
    DEFINE_BINARY_OPERATOR_ASSIGNMENT(/=)
    #undef DEFINE_BINARY_OPERATOR_ASSIGNMENT


    // += , -=, *=, /= with scalar
    #define DEFINE_OPERATOR_ASSIGNMENT(OP) \
        template <typename T, typename U, std::size_t N> \
        TinyVector<T, N>& operator OP(TinyVector<T, N>& vec, const U& scalar) \
        { \
            for (std::size_t i = 0; i < N; ++i) { \
                vec[i] OP scalar; \
            } \
            return vec; \
        }

    DEFINE_OPERATOR_ASSIGNMENT(*=)
    DEFINE_OPERATOR_ASSIGNMENT(+=)
    DEFINE_OPERATOR_ASSIGNMENT(-=)
    DEFINE_OPERATOR_ASSIGNMENT(/=)
    #undef DEFINE_OPERATOR_ASSIGNMENT

    

} // namespace ant