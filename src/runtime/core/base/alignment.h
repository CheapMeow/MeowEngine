#pragma once

#include <cstdint>

template<typename T>
inline constexpr T Align(T val, uint64_t alignment)
{
    return (T)(((uint64_t)val + alignment - 1) & ~(alignment - 1));
}

template<typename T>
inline constexpr T AlignDown(T val, uint64_t alignment)
{
    return (T)(((uint64_t)val) & ~(alignment - 1));
}

template<typename T>
inline constexpr bool IsAligned(T val, uint64_t alignment)
{
    return !((uint64_t)val & (alignment - 1));
}

template<typename T>
inline constexpr T AlignArbitrary(T val, uint64_t alignment)
{
    return (T)((((uint64_t)val + alignment - 1) / alignment) * alignment);
}