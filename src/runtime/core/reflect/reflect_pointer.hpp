#pragma once

#include "reflect.hpp"

namespace Meow
{
    namespace reflect
    {
        /**
         * @brief Pointer to base class. It is initialized by derived class instance, and it also stores derived class
         * name.
         *
         * Pointing to base class is to support polymorphism. Storing derived class name is to
         *
         * @tparam T Base class name.
         */
        template<typename T>
        struct refl_shared_ptr
        {
            refl_shared_ptr() {}

            refl_shared_ptr(refl_shared_ptr& rhs) noexcept
            {
                type_name  = rhs.type_name;
                shared_ptr = rhs.shared_ptr;
            }

            refl_shared_ptr& operator=(refl_shared_ptr& rhs) noexcept
            {
                if (this != &rhs)
                {
                    type_name  = rhs.type_name;
                    shared_ptr = rhs.shared_ptr;
                }

                return *this;
            }

            refl_shared_ptr(refl_shared_ptr&& rhs) noexcept
            {
                type_name = rhs.type_name;
                std::swap(shared_ptr, rhs.shared_ptr);
            }

            refl_shared_ptr& operator=(refl_shared_ptr&& rhs) noexcept
            {
                if (this != &rhs)
                {
                    type_name = rhs.type_name;
                    std::swap(shared_ptr, rhs.shared_ptr);
                }

                return *this;
            }

            std::string        type_name {""};
            std::shared_ptr<T> shared_ptr {nullptr};
        };
    } // namespace reflect

} // namespace Meow