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

            FieldAccessor(const FieldAccessor&)            = delete;
            FieldAccessor& operator=(const FieldAccessor&) = delete;
            FieldAccessor(FieldAccessor&&)                 = default;
            FieldAccessor& operator=(FieldAccessor&&)      = default;

            template<typename ClassType, typename FieldType>
            FieldAccessor(const std::string& name, const std::string& type_name, FieldType ClassType::*field_ptr)
                : m_name(name)
                , m_type_name(type_name)
            {
                m_ptr_getter = [field_ptr](std::any obj) -> void* {
                    return &(std::any_cast<ClassType*>(obj)->*field_ptr);
                };
                m_getter = [field_ptr](std::any obj) -> std::any { return std::any_cast<ClassType*>(obj)->*field_ptr; };
                m_setter = [field_ptr](std::any obj, std::any val) {
                    // Syntax: https://stackoverflow.com/a/670744/12003165
                    // `obj.*field`
                    auto* self       = std::any_cast<ClassType*>(obj);
                    self->*field_ptr = std::any_cast<FieldType>(val);
                };
            }

            const std::string& name() const { return m_name; }

            const std::string& type_name() const { return m_type_name; }

            template<typename ClassType>
            void* GetValuePtr(ClassType* ins_ptr) const
            {
                return m_ptr_getter(ins_ptr);
            }

            template<typename ClassType, typename FieldType>
            FieldType GetValue(ClassType* ins_ptr) const
            {
                return std::any_cast<FieldType>(m_getter(ins_ptr));
            }

            template<typename ClassType, typename FieldType>
            void SetValue(ClassType* ins_ptr, FieldType val)
            {
                m_setter(ins_ptr, val);
            }

        private:
            std::string                             m_name;
            std::string                             m_type_name;
            std::function<void*(std::any)>          m_ptr_getter {nullptr};
            std::function<std::any(std::any)>       m_getter {nullptr};
            std::function<void(std::any, std::any)> m_setter {nullptr};
        };

        class MethodAccessor
        {
        public:
            MethodAccessor() = default;

            MethodAccessor(const MethodAccessor&)            = delete;
            MethodAccessor& operator=(const MethodAccessor&) = delete;
            MethodAccessor(MethodAccessor&&)                 = default;
            MethodAccessor& operator=(MethodAccessor&&)      = default;

            template<typename ClassType, typename ReturnType, typename... Args>
            MethodAccessor(const std::string& name, ReturnType (ClassType::*func)(Args...))
                : m_name(name)
            {
                m_function = [this, func](std::any obj_args) -> std::any {
                    auto& warpped_args = *std::any_cast<std::array<UnsafeAny, sizeof...(Args) + 1>*>(obj_args);
                    auto  tuple_args   = UnwarpAsTuple<ClassType&, Args...>(warpped_args);
                    return std::apply(func, tuple_args);
                };
                m_args_number = sizeof...(Args);
            }

            template<typename ClassType, typename... Args>
            MethodAccessor(const std::string& name, void (ClassType::*func)(Args...))
                : m_name(name)
            {
                m_function = [this, func](std::any obj_args) -> std::any {
                    auto& warpped_args = *std::any_cast<std::array<UnsafeAny, sizeof...(Args) + 1>*>(obj_args);
                    auto  tuple_args   = UnwarpAsTuple<ClassType&, Args...>(warpped_args);
                    std::apply(func, tuple_args);
                    return std::any {};
                };
                m_args_number = sizeof...(Args);
            }

            template<typename ClassType, typename ReturnType, typename... Args>
            MethodAccessor(const std::string& name, ReturnType (ClassType::*func)(Args...) const)
                : m_name(name)
            {
                m_function = [this, func](std::any obj_args) -> std::any {
                    auto& warpped_args = *std::any_cast<std::array<UnsafeAny, sizeof...(Args) + 1>*>(obj_args);
                    auto  tuple_args   = UnwarpAsTuple<const ClassType&, Args...>(warpped_args);
                    return std::apply(func, tuple_args);
                };
                m_is_const    = true;
                m_args_number = sizeof...(Args);
            }

            template<typename ClassType, typename... Args>
            MethodAccessor(const std::string& name, void (ClassType::*func)(Args...) const)
                : m_name(name)
            {
                m_function = [this, func](std::any obj_args) -> std::any {
                    auto& warpped_args = *std::any_cast<std::array<UnsafeAny, sizeof...(Args) + 1>*>(obj_args);
                    auto  tuple_args   = UnwarpAsTuple<const ClassType&, Args...>(warpped_args);
                    std::apply(func, tuple_args);
                    return std::any {};
                };
                m_is_const    = true;
                m_args_number = sizeof...(Args);
            }

            const std::string& name() const { return m_name; }

            bool is_const() const { return m_is_const; }

            template<typename ClassType, typename... Args>
            std::any Invoke(ClassType& c, Args&&... args)
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
            std::string                       m_name;
            std::function<std::any(std::any)> m_function {nullptr};
            bool                              m_is_const {false};
            int                               m_args_number {0};
        };

        class TypeDescriptor
        {
        public:
            TypeDescriptor() {}

            TypeDescriptor(const std::string& name)
                : m_name(name)
            {}

            TypeDescriptor(const TypeDescriptor&)            = delete;
            TypeDescriptor& operator=(const TypeDescriptor&) = delete;
            TypeDescriptor(TypeDescriptor&&)                 = default;
            TypeDescriptor& operator=(TypeDescriptor&&)      = default;

            const std::string& name() const { return m_name; }

            const std::vector<FieldAccessor>& GetFields() const { return m_fields; }

            const std::vector<MethodAccessor>& GetMethods() const { return m_methods; }

            const FieldAccessor& GetField(const std::string& name) const
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

            const MethodAccessor& GetMethod(const std::string& name) const
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

            void AddField(FieldAccessor&& field) { m_fields.emplace_back(std::move(field)); }

            void AddMethod(MethodAccessor&& method) { m_methods.emplace_back(std::move(method)); }

        private:
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

            bool HasType(const std::string& name)
            {
                return m_type_descriptor_map.find(name) != m_type_descriptor_map.end();
            }

            const TypeDescriptor& GetType(const std::string& name) { return m_type_descriptor_map.find(name)->second; }

            void Register(TypeDescriptor&& desc)
            {
                if (HasType(desc.name()))
                    return;

                m_type_descriptor_map[desc.name()] = std::move(desc);
            }

        private:
            std::unordered_map<std::string, TypeDescriptor> m_type_descriptor_map;
        };
    } // namespace reflect
} // namespace Meow