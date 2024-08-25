#pragma once

#include <vector>

namespace Meow
{
    template<typename BitType>
    class BitMask
    {
    public:
        using MaskType = typename std::underlying_type<BitType>::type;

        constexpr BitMask() noexcept
            : m_mask(0)
        {}

        constexpr BitMask(BitType bit) noexcept
            : m_mask(static_cast<MaskType>(bit))
        {}

        constexpr BitMask(BitMask<BitType> const& rhs) noexcept = default;

        constexpr explicit BitMask(MaskType mask) noexcept
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

        explicit constexpr operator MaskType() const noexcept { return m_mask; }

        constexpr auto count() noexcept;

        constexpr std::vector<BitType> split() const noexcept;

        inline static BitMask<BitType> all_mask = BitType::ALL;

    private:
        MaskType m_mask;
    };

    template<typename BitType>
    constexpr auto BitMask<BitType>::count() noexcept
    {
        MaskType count = 0;
        MaskType m     = m_mask;
        while (m)
        {
            m &= m - 1; // Brian Kernighan
            ++count;
        }
        return count;
    }

    template<typename BitType>
    constexpr std::vector<BitType> BitMask<BitType>::split() const noexcept
    {
        std::vector<BitType> all_bits;

        MaskType count = BitMask<BitType>::all_mask.count();

        MaskType cur_mask = 0x1;
        for (MaskType i = 0; i < count; i++)
        {
            if (cur_mask & m_mask)
                all_bits.push_back(static_cast<BitType>(cur_mask));

            cur_mask = cur_mask << 1;
        }

        return all_bits;
    }
} // namespace Meow
