#include <any>
#include <array>
#include <functional>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <utility>

namespace Meow
{
    namespace reflect
    {
        class UnsafeAny
        {
        public:
            template<typename InputClass>
            UnsafeAny(InputClass&& val)
            {
                m_ref_type = std::is_reference_v<InputClass>;
                if (m_ref_type == 1)
                {
                    m_storage = &val;
                }
                else
                {
                    m_storage = new std::decay_t<InputClass>(val);
                }
            }

            ~UnsafeAny()
            {
                if (!m_ref_type)
                {
                    delete m_storage;
                }
            }

            template<typename OutputClass>
            OutputClass Cast()
            {
                using RawTptr = std::decay_t<OutputClass>*;
                return *static_cast<RawTptr>(m_storage);
            }

        private:
            void* m_storage {};
            int   m_ref_type {0};
        };

        template<typename... Args, size_t N, size_t... Is>
        std::tuple<Args...> UnwarpAsTuple(std::array<UnsafeAny, N>& arr, std::index_sequence<Is...>)
        {
            // Dont use std::make_tuple, which can't easily support reference.
            return std::forward_as_tuple(arr[Is].template Cast<Args>()...);
        }

        template<typename... Args, size_t N, typename = std::enable_if_t<(N == sizeof...(Args))>>
        std::tuple<Args...> UnwarpAsTuple(std::array<UnsafeAny, N>& arr)
        {
            return UnwarpAsTuple<Args...>(arr, std::make_index_sequence<N> {});
        }
    } // namespace reflect
} // namespace Meow