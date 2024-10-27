#pragma once

#include <concepts>
#include <vector>

namespace Meow
{
    template<std::integral T>
    constexpr int count_bits(T n)
    {
        int count = 0;
        while (n)
        {
            n &= n - 1; // Brian Kernighan
            ++count;
        }
        return count;
    }

    template<typename BitType>
    class BitMask
    {
    public:
        using UnderlyingType = typename std::underlying_type<BitType>::type;

        constexpr BitMask() noexcept
            : m_mask(0)
        {}

        constexpr BitMask(BitType bit) noexcept
            : m_mask(static_cast<UnderlyingType>(bit))
        {}

        constexpr BitMask(BitMask<BitType> const& rhs) noexcept = default;

        constexpr explicit BitMask(UnderlyingType mask) noexcept
            : m_mask(mask)
        {}

        // relational operators
        auto operator<=>(BitMask<BitType> const&) const = default;

        // logical operator
        constexpr bool operator!() const noexcept { return !m_mask; }

        // bitwise operators
        constexpr BitMask<BitType> operator&(BitMask<BitType> const& rhs) const noexcept
        {
            return BitMask<BitType>(m_mask & rhs.m_mask);
        }

        constexpr BitMask<BitType> operator|(BitMask<BitType> const& rhs) const noexcept
        {
            return BitMask<BitType>(m_mask | rhs.m_mask);
        }

        constexpr BitMask<BitType> operator^(BitMask<BitType> const& rhs) const noexcept
        {
            return BitMask<BitType>(m_mask ^ rhs.m_mask);
        }

        constexpr BitMask<BitType> operator~() const noexcept
        {
            return BitMask<BitType>(m_mask ^ BitMask<BitType>::all_mask.m_mask);
        }

        // assignment operators
        constexpr BitMask<BitType>& operator=(BitMask<BitType> const& rhs) noexcept = default;

        constexpr BitMask<BitType>& operator|=(BitMask<BitType> const& rhs) noexcept
        {
            m_mask |= rhs.m_mask;
            return *this;
        }

        constexpr BitMask<BitType>& operator&=(BitMask<BitType> const& rhs) noexcept
        {
            m_mask &= rhs.m_mask;
            return *this;
        }

        constexpr BitMask<BitType>& operator^=(BitMask<BitType> const& rhs) noexcept
        {
            m_mask ^= rhs.m_mask;
            return *this;
        }

        // cast operators
        explicit constexpr operator bool() const noexcept { return !!m_mask; }

        explicit constexpr operator UnderlyingType() const noexcept { return m_mask; }

        constexpr int count() const noexcept;

        constexpr std::vector<BitType> split() const noexcept;

        constexpr std::vector<int> split_pos() const noexcept;

        constexpr static UnderlyingType all_mask       = static_cast<UnderlyingType>(BitType::ALL);
        constexpr static int            all_mask_count = count_bits(all_mask);

    private:
        UnderlyingType m_mask;
    };

    template<typename BitType>
    constexpr int BitMask<BitType>::count() const noexcept
    {
        return count_bits(m_mask);
    }

    template<typename BitType>
    constexpr std::vector<BitType> BitMask<BitType>::split() const noexcept
    {
        std::vector<BitType> all_bits;

        UnderlyingType cur_mask = 0x1;
        for (UnderlyingType i = 0; i < BitMask<BitType>::all_mask_count; i++)
        {
            if (cur_mask & m_mask)
                all_bits.push_back(static_cast<BitType>(cur_mask));

            cur_mask = cur_mask << 1;
        }

        return all_bits;
    }

    template<typename BitType>
    constexpr std::vector<int> BitMask<BitType>::split_pos() const noexcept
    {
        std::vector<int> all_pos;

        UnderlyingType cur_mask = 0x1;
        for (UnderlyingType i = 0; i < BitMask<BitType>::all_mask_count; i++)
        {
            if (cur_mask & m_mask)
                all_pos.push_back(i);

            cur_mask = cur_mask << 1;
        }

        return all_pos;
    }
} // namespace Meow
