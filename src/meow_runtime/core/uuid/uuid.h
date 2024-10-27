#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace Meow
{
    class UUID
    {
    public:
        UUID();
        UUID(uint64_t uuid);
        UUID(const UUID&)                             = default;
        UUID&     operator=(const UUID& rhs) noexcept = default;
        constexpr operator uint64_t() const noexcept { return m_UUID; }

    private:
        uint64_t m_UUID;
    };

} // namespace Meow

namespace std
{
    template<typename T>
    struct hash;

    template<>
    struct hash<Meow::UUID>
    {
        std::size_t operator()(const Meow::UUID& uuid) const { return (uint64_t)uuid; }
    };

} // namespace std
