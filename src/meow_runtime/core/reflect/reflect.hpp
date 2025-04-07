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

            template<typename ClassType, typename FieldType>
            FieldAccessor(const std::string& name, const std::string& type_name, FieldType ClassType::*field_ptr)
            {
                m_name      = name;
                m_type_name = type_name;

                m_getter = [field_ptr](void* obj) -> void* {
                    ClassType* self = static_cast<ClassType*>(obj);
                    return &(self->*field_ptr);
                };

                m_setter = [field_ptr](void* obj, void* val) {
                    ClassType* self  = static_cast<ClassType*>(obj);
                    self->*field_ptr = *static_cast<FieldType*>(val);
                };
            }

            const std::string& name() const { return m_name; }

            const std::string& type_name() const { return m_type_name; }

            void* get(void* ins_ptr) const { return m_getter(ins_ptr); }

            void set(void* ins_ptr, void* val) const { m_setter(ins_ptr, val); }

        private:
            std::string m_name;
            std::string m_type_name;

            std::function<void*(void*)>       m_getter {nullptr};
            std::function<void(void*, void*)> m_setter {nullptr};
        };

        class ArrayAccessor
        {
        public:
            ArrayAccessor() = default;

            template<typename ClassType, typename InnerType>
            ArrayAccessor(const std::string&     name,
                          const std::string&     type_name,
                          const std::string&     inner_type_name,
                          std::vector<InnerType> ClassType::*array_ptr)
            {
                m_name            = name;
                m_type_name       = type_name;
                m_inner_type_name = inner_type_name;

                m_getter = [array_ptr](void* obj, std::size_t idx) -> void* {
                    ClassType*             self = static_cast<ClassType*>(obj);
                    std::vector<InnerType> arr  = static_cast<std::vector<InnerType>>(self->*array_ptr);
                    return &(arr[idx]);
                };

                m_setter = [array_ptr](void* obj, void* val, std::size_t idx) {
                    ClassType*             self = static_cast<ClassType*>(obj);
                    std::vector<InnerType> arr  = static_cast<std::vector<InnerType>>(self->*array_ptr);
                    arr[idx]                    = *static_cast<InnerType*>(val);
                };

                m_array_size_getter = [array_ptr](void* obj) -> std::size_t {
                    ClassType*             self = static_cast<ClassType*>(obj);
                    std::vector<InnerType> arr  = static_cast<std::vector<InnerType>>(self->*array_ptr);
                    return arr.size();
                };
            }

            const std::string& name() const { return m_name; }

            const std::string& type_name() const { return m_type_name; }

            const std::string& inner_type_name() const { return m_inner_type_name; }

            void* get(void* ins_ptr, std::size_t idx = 0) const { return m_getter(ins_ptr, idx); }

            void set(void* ins_ptr, void* val, std::size_t idx = 0) const { m_setter(ins_ptr, val, idx); }

            const std::size_t get_size(void* ins_ptr) const { return m_array_size_getter(ins_ptr); }

        private:
            std::string m_name;
            std::string m_type_name;
            std::string m_inner_type_name;

            std::function<void*(void*, std::size_t)>       m_getter {nullptr};
            std::function<void(void*, void*, std::size_t)> m_setter {nullptr};
            std::function<std::size_t(void*)>              m_array_size_getter {nullptr};
        };

        class MethodAccessor
        {
        public:
            MethodAccessor() = default;

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
            std::any Invoke(ClassType& c, Args&&... args) const
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

            const std::vector<ArrayAccessor>& GetArrays() const { return m_arrays; }

            const std::vector<MethodAccessor>& GetMethods() const { return m_methods; }

            void AddField(FieldAccessor&& field) { m_fields.emplace_back(std::move(field)); }

            void AddArray(ArrayAccessor&& array) { m_arrays.emplace_back(std::move(array)); }

            void AddMethod(MethodAccessor&& method) { m_methods.emplace_back(std::move(method)); }

        private:
            std::string                 m_name;
            std::vector<FieldAccessor>  m_fields;
            std::vector<ArrayAccessor>  m_arrays;
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