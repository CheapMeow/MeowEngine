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
            refl_shared_ptr(const std::string& name, std::shared_ptr<T> ptr)
                : type_name(name)
                , shared_ptr(ptr)
            {}

            refl_shared_ptr() {}

            refl_shared_ptr(const refl_shared_ptr&)            = default;
            refl_shared_ptr& operator=(const refl_shared_ptr&) = default;
            refl_shared_ptr(refl_shared_ptr&&)                 = default;
            refl_shared_ptr& operator=(refl_shared_ptr&&)      = default;

            std::string        type_name {""};
            std::shared_ptr<T> shared_ptr {nullptr};
        };
    } // namespace reflect

} // namespace Meow
