#pragma once

#include <cstdint>   // For uint64_t, uintptr_t
#include <cstddef>   // For size_t, byte, std::align_val_t (C++17)
#include <type_traits> // For std::is_integral, std::is_pointer, std::is_base_of
#include <cassert>   // For assert()
#include <limits>    // For std::numeric_limits
#include <new>       // For std::hardware_destructive_interference_size etc. (C++17)
#include <cstdlib>   // For aligned_alloc (if available), free

#ifdef _WIN32
#include <malloc.h>  // For _aligned_malloc, _aligned_free
#else
#include <stdlib.h> // For posix_memalign, free (or already included via cstdlib)
#endif


// Define a namespace to encapsulate the alignment utilities
namespace align_util
{
    // --- Compile-Time Constants (C++17) ---

    // Useful alignment constants, often related to cache lines
#ifdef __cpp_lib_hardware_interference_size
    inline constexpr size_t CacheLineSize = std::hardware_destructive_interference_size;
    // inline constexpr size_t CacheLineAlignment = std::hardware_constructive_interference_size; // Also available
#else
    // Provide a reasonable fallback if the standard constants aren't available
    // Warning: This is a guess and might not be optimal for the target architecture.
    inline constexpr size_t CacheLineSize = 64;
#endif


    // --- Helper Functions ---

    /**
     * @brief Checks if a number is a power of two.
     * @param n The number to check.
     * @return true if n is a power of two, false otherwise. Returns false for n = 0.
     */
    inline constexpr bool IsPowerOfTwo(uint64_t n) noexcept
    {
        return (n != 0) && ((n & (n - 1)) == 0);
    }

    /**
    * @brief Calculates the smallest power of two greater than or equal to n.
    * Returns 0 if n is 0 or if the result would overflow uint64_t.
    * @param n The input number.
    * @return The next power of two.
    */
    inline constexpr uint64_t NextPowerOfTwo(uint64_t n) noexcept
    {
        if (n == 0) return 1; // Common convention, though debatable
        n--; // clang-tidy recommends this approach to handle exact powers of two correctly
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n |= n >> 32;
        n++;
        // Handle potential overflow if n was already > half max_uint64
        // Note: This check isn't perfect in constexpr before C++20 without intrinsics
        if (n == 0) return 0; // Indicates overflow occurred during increment
        return n;
    }


    // --- Core Alignment Calculations (Templated for Integral Types) ---

    /**
     * @brief Aligns a value up to the nearest multiple of a power-of-two alignment.
     * @tparam T An integral type.
     * @param val The value to align.
     * @param alignment The alignment boundary (must be > 0 and a power of two).
     * @return The aligned value.
     */
    template<typename T>
    inline constexpr T AlignUpPow2(T val, size_t alignment) noexcept
    {
        static_assert(std::is_integral_v<T>, "AlignUpPow2 requires an integral type.");
        assert(alignment > 0 && "Alignment must be greater than 0.");
        assert(IsPowerOfTwo(alignment) && "Alignment must be a power of two for AlignUpPow2.");
        // Use unsigned arithmetic internally to ensure defined bitwise behavior
        using U = std::make_unsigned_t<T>;
        // Use uintptr_t for intermediate calculation if T could be large
        using CalcT = std::conditional_t<(sizeof(T) > sizeof(uintptr_t)), uint64_t, uintptr_t>;
        // Ensure CalcT is large enough
        static_assert(sizeof(CalcT) >= sizeof(T), "Calculation type too small.");

        return static_cast<T>((static_cast<CalcT>(static_cast<U>(val)) + static_cast<CalcT>(alignment) - 1) & ~(static_cast<CalcT>(alignment) - 1));
    }

    /**
     * @brief Aligns a value down to the nearest multiple of a power-of-two alignment.
     * @tparam T An integral type.
     * @param val The value to align.
     * @param alignment The alignment boundary (must be > 0 and a power of two).
     * @return The aligned value.
     */
    template<typename T>
    inline constexpr T AlignDownPow2(T val, size_t alignment) noexcept
    {
        static_assert(std::is_integral_v<T>, "AlignDownPow2 requires an integral type.");
        assert(alignment > 0 && "Alignment must be greater than 0.");
        assert(IsPowerOfTwo(alignment) && "Alignment must be a power of two for AlignDownPow2.");
        using U = std::make_unsigned_t<T>;
        using CalcT = std::conditional_t<(sizeof(T) > sizeof(uintptr_t)), uint64_t, uintptr_t>;
        static_assert(sizeof(CalcT) >= sizeof(T), "Calculation type too small.");

        return static_cast<T>(static_cast<CalcT>(static_cast<U>(val)) & ~(static_cast<CalcT>(alignment) - 1));
    }

    /**
     * @brief Checks if a value is aligned to a power-of-two alignment boundary.
     * @tparam T An integral type.
     * @param val The value to check.
     * @param alignment The alignment boundary (must be > 0 and a power of two).
     * @return true if the value is aligned, false otherwise.
     */
    template<typename T>
    inline constexpr bool IsAlignedPow2(T val, size_t alignment) noexcept
    {
        static_assert(std::is_integral_v<T>, "IsAlignedPow2 requires an integral type.");
        assert(alignment > 0 && "Alignment must be greater than 0.");
        assert(IsPowerOfTwo(alignment) && "Alignment must be a power of two for IsAlignedPow2.");
        using U = std::make_unsigned_t<T>;
        using CalcT = std::conditional_t<(sizeof(T) > sizeof(uintptr_t)), uint64_t, uintptr_t>;
        static_assert(sizeof(CalcT) >= sizeof(T), "Calculation type too small.");

        return !(static_cast<CalcT>(static_cast<U>(val)) & (static_cast<CalcT>(alignment) - 1));
    }


    /**
     * @brief Aligns a value up to the nearest multiple of an arbitrary alignment.
     * Slower than AlignUpPow2 for power-of-two alignments due to division.
     * @tparam T An integral type.
     * @param val The value to align.
     * @param alignment The alignment boundary (must be > 0).
     * @return The aligned value.
     */
    template<typename T>
    inline constexpr T AlignUpArbitrary(T val, size_t alignment) noexcept
    {
        static_assert(std::is_integral_v<T>, "AlignUpArbitrary requires an integral type.");
        assert(alignment > 0 && "Alignment must be greater than 0.");

        // Handle power-of-two case efficiently if possible (optimization)
        if (IsPowerOfTwo(alignment)) {
             return AlignUpPow2(val, alignment);
        }

        using U = std::make_unsigned_t<T>;
        using CalcT = std::conditional_t<(sizeof(T) > sizeof(uint64_t)), unsigned __int128, uint64_t>; // Use largest practical type
        static_assert(sizeof(CalcT) >= sizeof(T), "Calculation type too small.");

        CalcT u_val = static_cast<CalcT>(static_cast<U>(val));
        CalcT u_align = static_cast<CalcT>(alignment);
        CalcT remainder = u_val % u_align;

        if (remainder == 0) {
            return val; // Already aligned
        }

        // Add padding to reach the next boundary
        // Check for potential overflow before adding padding
        CalcT padding = u_align - remainder;
        if (u_val > std::numeric_limits<CalcT>::max() - padding) {
             // Overflow would occur. This case is complex in constexpr.
             // For runtime, one might throw or assert. In constexpr, maybe assert.
             // C++20 constexpr allows more, but for broader compatibility, assert.
             assert(false && "Overflow detected during arbitrary alignment.");
             // Or return max value? Or rely on UB? Assert is clearest intent.
             return std::numeric_limits<T>::max(); // Or some error indication
        }
        return static_cast<T>(u_val + padding);

        // Original less safe arbitrary formula (prone to overflow on intermediate add):
        // return (T)((((uint64_t)val + alignment - 1) / alignment) * alignment);
        // Alternative using remainder (safer):
        // uint64_t remainder = (uint64_t)val % alignment;
        // return (remainder == 0) ? val : (T)((uint64_t)val + alignment - remainder);
    }

     /**
     * @brief Aligns a value down to the nearest multiple of an arbitrary alignment.
     * Slower than AlignDownPow2 for power-of-two alignments.
     * @tparam T An integral type.
     * @param val The value to align.
     * @param alignment The alignment boundary (must be > 0).
     * @return The aligned value.
     */
    template<typename T>
    inline constexpr T AlignDownArbitrary(T val, size_t alignment) noexcept
    {
        static_assert(std::is_integral_v<T>, "AlignDownArbitrary requires an integral type.");
        assert(alignment > 0 && "Alignment must be greater than 0.");

        if (IsPowerOfTwo(alignment)) {
             return AlignDownPow2(val, alignment);
        }

        using U = std::make_unsigned_t<T>;
        using CalcT = std::conditional_t<(sizeof(T) > sizeof(uint64_t)), unsigned __int128, uint64_t>;
        static_assert(sizeof(CalcT) >= sizeof(T), "Calculation type too small.");

        CalcT u_val = static_cast<CalcT>(static_cast<U>(val));
        CalcT u_align = static_cast<CalcT>(alignment);
        CalcT remainder = u_val % u_align;

        return static_cast<T>(u_val - remainder);
    }

    /**
     * @brief Checks if a value is aligned to an arbitrary alignment boundary.
     * @tparam T An integral type.
     * @param val The value to check.
     * @param alignment The alignment boundary (must be > 0).
     * @return true if the value is aligned, false otherwise.
     */
    template<typename T>
    inline constexpr bool IsAlignedArbitrary(T val, size_t alignment) noexcept
    {
        static_assert(std::is_integral_v<T>, "IsAlignedArbitrary requires an integral type.");
        assert(alignment > 0 && "Alignment must be greater than 0.");

        if (IsPowerOfTwo(alignment)) {
            return IsAlignedPow2(val, alignment);
        }

        using U = std::make_unsigned_t<T>;
        using CalcT = std::conditional_t<(sizeof(T) > sizeof(uint64_t)), unsigned __int128, uint64_t>;
        static_assert(sizeof(CalcT) >= sizeof(T), "Calculation type too small.");

        return (static_cast<CalcT>(static_cast<U>(val)) % static_cast<CalcT>(alignment)) == 0;
    }


    // --- Offset Calculation ---

    /**
     * @brief Calculates the offset needed to add to 'val' to reach the next power-of-two alignment boundary.
     * Returns 0 if 'val' is already aligned.
     * @tparam T An integral type.
     * @param val The base value.
     * @param alignment The alignment boundary (must be > 0 and a power of two).
     * @return The offset (padding) needed to reach alignment.
     */
    template<typename T>
    inline constexpr size_t OffsetToAlignmentPow2(T val, size_t alignment) noexcept
    {
        static_assert(std::is_integral_v<T>, "OffsetToAlignmentPow2 requires an integral type.");
        assert(alignment > 0 && "Alignment must be greater than 0.");
        assert(IsPowerOfTwo(alignment) && "Alignment must be a power of two.");

        // uintptr_t is suitable for offsets/sizes
        uintptr_t addr = reinterpret_cast<uintptr_t>(static_cast<std::make_unsigned_t<T>>(val));
        uintptr_t mask = static_cast<uintptr_t>(alignment) - 1;
        uintptr_t misalignment = addr & mask;

        // If misalignment is 0, offset is 0. Otherwise, it's alignment - misalignment.
        return static_cast<size_t>((alignment - misalignment) & mask);
        // Alternative: return (size_t)(AlignUpPow2(val, alignment) - val); but requires checking T fits size_t if T is large.
    }

     /**
     * @brief Calculates the offset needed to add to 'val' to reach the next arbitrary alignment boundary.
     * Returns 0 if 'val' is already aligned.
     * @tparam T An integral type.
     * @param val The base value.
     * @param alignment The alignment boundary (must be > 0).
     * @return The offset (padding) needed to reach alignment.
     */
    template<typename T>
    inline constexpr size_t OffsetToAlignmentArbitrary(T val, size_t alignment) noexcept
    {
         static_assert(std::is_integral_v<T>, "OffsetToAlignmentArbitrary requires an integral type.");
         assert(alignment > 0 && "Alignment must be greater than 0.");

         if (IsPowerOfTwo(alignment)) {
             return OffsetToAlignmentPow2(val, alignment);
         }

         using U = std::make_unsigned_t<T>;
         using CalcT = std::conditional_t<(sizeof(T) > sizeof(uint64_t)), unsigned __int128, uint64_t>;
         static_assert(sizeof(CalcT) >= sizeof(T), "Calculation type too small.");

         CalcT u_val = static_cast<CalcT>(static_cast<U>(val));
         CalcT u_align = static_cast<CalcT>(alignment);
         CalcT remainder = u_val % u_align;

         return static_cast<size_t>((remainder == 0) ? 0 : (u_align - remainder));
    }


    // --- Pointer Alignment ---

    /**
     * @brief Aligns a pointer up to the nearest multiple of a power-of-two alignment.
     * @tparam T The type pointed to (can be void).
     * @param ptr The pointer to align.
     * @param alignment The alignment boundary (must be > 0 and a power of two).
     * @return The aligned pointer.
     */
    template <typename T>
    inline constexpr T* AlignUpPow2(T* ptr, size_t alignment) noexcept
    {
        // Use uintptr_t for pointer arithmetic
        return reinterpret_cast<T*>(AlignUpPow2(reinterpret_cast<uintptr_t>(ptr), alignment));
    }

    /**
     * @brief Aligns a pointer down to the nearest multiple of a power-of-two alignment.
     * @tparam T The type pointed to (can be void).
     * @param ptr The pointer to align.
     * @param alignment The alignment boundary (must be > 0 and a power of two).
     * @return The aligned pointer.
     */
    template <typename T>
    inline constexpr T* AlignDownPow2(T* ptr, size_t alignment) noexcept
    {
        return reinterpret_cast<T*>(AlignDownPow2(reinterpret_cast<uintptr_t>(ptr), alignment));
    }

    /**
     * @brief Checks if a pointer is aligned to a power-of-two alignment boundary.
     * @tparam T The type pointed to (can be void).
     * @param ptr The pointer to check.
     * @param alignment The alignment boundary (must be > 0 and a power of two).
     * @return true if the pointer is aligned, false otherwise.
     */
    template <typename T>
    inline constexpr bool IsAlignedPow2(T* ptr, size_t alignment) noexcept
    {
        return IsAlignedPow2(reinterpret_cast<uintptr_t>(ptr), alignment);
    }

    /**
     * @brief Aligns a pointer up to the nearest multiple of an arbitrary alignment.
     * @tparam T The type pointed to (can be void).
     * @param ptr The pointer to align.
     * @param alignment The alignment boundary (must be > 0).
     * @return The aligned pointer.
     */
    template <typename T>
    inline /*constexpr*/ T* AlignUpArbitrary(T* ptr, size_t alignment) noexcept // Not easily constexpr pre C++20
    {
        // Cannot easily be constexpr before C++20 due to reinterpret_cast limitations in constexpr
        // and potential overflow checks in arbitrary alignment.
        return reinterpret_cast<T*>(AlignUpArbitrary(reinterpret_cast<uintptr_t>(ptr), alignment));
    }

    /**
     * @brief Aligns a pointer down to the nearest multiple of an arbitrary alignment.
     * @tparam T The type pointed to (can be void).
     * @param ptr The pointer to align.
     * @param alignment The alignment boundary (must be > 0).
     * @return The aligned pointer.
     */
    template <typename T>
    inline /*constexpr*/ T* AlignDownArbitrary(T* ptr, size_t alignment) noexcept // Not easily constexpr pre C++20
    {
        return reinterpret_cast<T*>(AlignDownArbitrary(reinterpret_cast<uintptr_t>(ptr), alignment));
    }

    /**
     * @brief Checks if a pointer is aligned to an arbitrary alignment boundary.
     * @tparam T The type pointed to (can be void).
     * @param ptr The pointer to check.
     * @param alignment The alignment boundary (must be > 0).
     * @return true if the pointer is aligned, false otherwise.
     */
    template <typename T>
    inline constexpr bool IsAlignedArbitrary(T* ptr, size_t alignment) noexcept
    {
        return IsAlignedArbitrary(reinterpret_cast<uintptr_t>(ptr), alignment);
    }

    /**
     * @brief Calculates the offset needed to add to 'ptr' to reach the next power-of-two alignment boundary.
     * @tparam T The type pointed to (can be void).
     * @param ptr The base pointer.
     * @param alignment The alignment boundary (must be > 0 and a power of two).
     * @return The offset (padding) in bytes needed to reach alignment.
     */
    template <typename T>
    inline constexpr size_t OffsetToAlignmentPow2(T* ptr, size_t alignment) noexcept
    {
        return OffsetToAlignmentPow2(reinterpret_cast<uintptr_t>(ptr), alignment);
    }

     /**
     * @brief Calculates the offset needed to add to 'ptr' to reach the next arbitrary alignment boundary.
     * @tparam T The type pointed to (can be void).
     * @param ptr The base pointer.
     * @param alignment The alignment boundary (must be > 0).
     * @return The offset (padding) in bytes needed to reach alignment.
     */
    template <typename T>
    inline constexpr size_t OffsetToAlignmentArbitrary(T* ptr, size_t alignment) noexcept
    {
         return OffsetToAlignmentArbitrary(reinterpret_cast<uintptr_t>(ptr), alignment);
    }


    // --- Aligned Memory Allocation Wrappers ---
    // Provides a consistent interface over platform-specific functions.

    /**
     * @brief Allocates memory with specified alignment.
     * @param size The number of bytes to allocate.
     * @param alignment The desired alignment (must be > 0 and a power of two).
     * @return A pointer to the allocated aligned memory, or nullptr on failure.
     * Memory should be freed using FreeAligned.
     */
    inline void* AllocateAligned(size_t size, size_t alignment) noexcept
    {
        assert(alignment > 0 && IsPowerOfTwo(alignment) && "Alignment must be > 0 and a power of two.");
        void* ptr = nullptr;

#if defined(__cpp_aligned_new) && __cpp_aligned_new >= 201606L && !defined(ALIGN_UTIL_FORCE_MALLOC)
        // C++17 aligned new (use if available and not forced otherwise)
        // Note: Requires operator delete with std::align_val_t overload for freeing.
        // This simple wrapper might hide that complexity, consider carefully.
        // For general purpose allocation, platform functions are often easier.
        // ptr = ::operator new(size, std::align_val_t{alignment}, std::nothrow);

        // Let's stick to malloc-style wrappers for simpler FreeAligned counterpart
#endif

#if defined(_WIN32)
        ptr = _aligned_malloc(size, alignment);
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && defined(__STDC_LIB_EXT1__) || defined(__linux__) || defined(__APPLE__)
        // Use aligned_alloc if C11 + Annex K, or on Linux/macOS (usually available)
        // Note: aligned_alloc requires size to be a multiple of alignment.
        size_t aligned_size = AlignUpPow2(size, alignment); // Ensure size is multiple
        if (aligned_size < size) return nullptr; // Check overflow
        ptr = aligned_alloc(alignment, aligned_size);
#else
        // Fallback using posix_memalign if available (common on Unix-like systems)
        // Requires `#include <stdlib.h>` or equivalent.
        if (posix_memalign(&ptr, alignment, size) != 0) {
            ptr = nullptr; // posix_memalign failed
        }
#endif

        // Final fallback - over-allocate and align manually (less efficient, error prone)
        // Not implemented here for brevity - prefer platform functions.

        return ptr;
    }

    /**
     * @brief Frees memory previously allocated with AllocateAligned.
     * @param ptr Pointer to the memory block to free.
     */
    inline void FreeAligned(void* ptr) noexcept
    {
        if (!ptr) return;

#if defined(_WIN32)
        _aligned_free(ptr);
#else
        // Standard free works for posix_memalign and aligned_alloc
        free(ptr);
#endif
    }

    // --- Simple Aligned Deleter for Smart Pointers ---
    struct AlignedDeleter
    {
        void operator()(void* ptr) const noexcept {
            FreeAligned(ptr);
        }
    };

    template<typename T>
    using AlignedUniquePtr = std::unique_ptr<T, AlignedDeleter>;

    // Example: AlignedUniquePtr<MyStruct> ptr(static_cast<MyStruct*>(AllocateAligned(sizeof(MyStruct), alignof(MyStruct))));


    // --- Platform Specific Alignment Macros (Example) ---
    // Use alignas(N) in C++11 and later where possible.
    // These are fallbacks or for C compatibility / specific compiler features.
    #ifdef _MSC_VER
        #define ALIGNAS_MSVC(N) __declspec(align(N))
        #define ALIGNAS_GCC(N) /* Nothing */
    #elif defined(__GNUC__) || defined(__clang__)
        #define ALIGNAS_MSVC(N) /* Nothing */
        #define ALIGNAS_GCC(N) __attribute__((aligned(N)))
    #else
        #define ALIGNAS_MSVC(N) /* Nothing */
        #define ALIGNAS_GCC(N) /* Nothing */
    #endif

    // Preferred C++11 way: alignas(N)


} // namespace align_util


// --- Restore original definitions if they existed ---
// (Example - less common for such basic utilities)
// #pragma pop_macro("Align") // If macros were used/overridden
