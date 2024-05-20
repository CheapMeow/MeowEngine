#pragma once

#include "unsafe_any.hpp"

#include <any>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace Meow
{
    namespace reflect
    {
        class FieldAccessor
        {
        public:
            FieldAccessor() = default;

            template<typename C, typename T>
            FieldAccessor(T C::*var)
            {
                m_getter = [var](std::any obj) -> std::any { return std::any_cast<const C*>(obj)->*var; };
                m_setter = [var](std::any obj, std::any val) {
                    // Syntax: https://stackoverflow.com/a/670744/12003165
                    // `obj.*field`
                    auto* self = std::any_cast<C*>(obj);
                    self->*var = std::any_cast<T>(val);
                };
            }

            const std::string& name() const { return m_name; }

            template<typename T, typename C>
            T GetValue(const C& c) const
            {
                return std::any_cast<T>(m_getter(&c));
            }

            template<typename C, typename T>
            void SetValue(C& c, T val)
            {
                m_setter(&c, val);
            }

        private:
            friend class RawTypeDescriptorBuilder;

            std::string                             m_name;
            std::function<std::any(std::any)>       m_getter {nullptr};
            std::function<void(std::any, std::any)> m_setter {nullptr};
        };

        class MethodAccessor
        {
        public:
            MethodAccessor() = default;

            template<typename C, typename R, typename... Args>
            explicit MethodAccessor(R (C::*func)(Args...))
            {
                m_function = [this, func](std::any obj_args) -> std::any {
                    auto& warpped_args = *std::any_cast<std::array<UnsafeAny, sizeof...(Args) + 1>*>(obj_args);
                    auto  tuple_args   = UnwarpAsTuple<C&, Args...>(warpped_args);
                    return std::apply(func, tuple_args);
                };
                m_args_number = sizeof...(Args);
            }

            template<typename C, typename... Args>
            explicit MethodAccessor(void (C::*func)(Args...))
            {
                m_function = [this, func](std::any obj_args) -> std::any {
                    auto& warpped_args = *std::any_cast<std::array<UnsafeAny, sizeof...(Args) + 1>*>(obj_args);
                    auto  tuple_args   = UnwarpAsTuple<C&, Args...>(warpped_args);
                    std::apply(func, tuple_args);
                    return std::any {};
                };
                m_args_number = sizeof...(Args);
            }

            template<typename C, typename R, typename... Args>
            explicit MethodAccessor(R (C::*func)(Args...) const)
            {
                m_function = [this, func](std::any obj_args) -> std::any {
                    auto& warpped_args = *std::any_cast<std::array<UnsafeAny, sizeof...(Args) + 1>*>(obj_args);
                    auto  tuple_args   = UnwarpAsTuple<const C&, Args...>(warpped_args);
                    return std::apply(func, tuple_args);
                };
                m_is_const    = true;
                m_args_number = sizeof...(Args);
            }

            template<typename C, typename... Args>
            explicit MethodAccessor(void (C::*func)(Args...) const)
            {
                m_function = [this, func](std::any obj_args) -> std::any {
                    auto& warpped_args = *std::any_cast<std::array<UnsafeAny, sizeof...(Args) + 1>*>(obj_args);
                    auto  tuple_args   = UnwarpAsTuple<const C&, Args...>(warpped_args);
                    std::apply(func, tuple_args);
                    return std::any {};
                };
                m_is_const    = true;
                m_args_number = sizeof...(Args);
            }

            const std::string& name() const { return m_name; }

            bool is_const() const { return m_is_const; }

            template<typename C, typename... Args>
            std::any Invoke(C& c, Args&&... args)
            {
                if (m_args_number != sizeof...(Args))
                {
                    throw std::runtime_error("Mismatching number of args!");
                }

                if (m_is_const)
                {
                    std::array<UnsafeAny, sizeof...(Args) + 1> args_arr = {UnsafeAny {c},
                                                                           UnsafeAny {std::forward<Args>(args)}...};
                    return m_function(&args_arr);
                }
                else
                {
                    std::array<UnsafeAny, sizeof...(Args) + 1> args_arr = {UnsafeAny {c},
                                                                           UnsafeAny {std::forward<Args>(args)}...};
                    return m_function(&args_arr);
                }
            }

        private:
            friend class RawTypeDescriptorBuilder;

            std::string                       m_name;
            std::function<std::any(std::any)> m_function {nullptr};
            bool                              m_is_const {false};
            int                               m_args_number {0};
        };

        class TypeDescriptor
        {
        public:
            const std::string& name() const { return m_name; }

            const std::vector<FieldAccessor>& fields() const { return m_fields; }

            const std::vector<MethodAccessor>& methods() const { return m_methods; }

            FieldAccessor GetField(const std::string& name) const
            {
                for (const auto& field : m_fields)
                {
                    if (field.name() == name)
                    {
                        return field;
                    }
                }
                return FieldAccessor {};
            }

            MethodAccessor GetMethod(const std::string& name) const
            {
                for (const auto& method : m_methods)
                {
                    if (method.name() == name)
                    {
                        return method;
                    }
                }
                return MethodAccessor {};
            }

        private:
            friend class RawTypeDescriptorBuilder;

            std::string                 m_name;
            std::vector<FieldAccessor>  m_fields;
            std::vector<MethodAccessor> m_methods;
        };

        class Registry
        {
        public:
            static Registry& instance()
            {
                static Registry inst;
                return inst;
            }

            TypeDescriptor* Find(const std::string& name);

            void Register(std::unique_ptr<TypeDescriptor> desc);

            void Clear();

        private:
            std::unordered_map<std::string, std::unique_ptr<TypeDescriptor>> m_type_descriptor_map;
        };

        TypeDescriptor& GetByName(const std::string& name);

        void ClearRegistry();

    } // namespace reflect
} // namespace Meow