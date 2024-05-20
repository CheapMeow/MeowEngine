#pragma once

#include "reflect.hpp"

namespace Meow
{
    namespace reflect
    {
        class RawTypeDescriptorBuilder
        {
        public:
            explicit RawTypeDescriptorBuilder(const std::string& name);

            ~RawTypeDescriptorBuilder();
            RawTypeDescriptorBuilder(const RawTypeDescriptorBuilder&)            = delete;
            RawTypeDescriptorBuilder& operator=(const RawTypeDescriptorBuilder&) = delete;
            RawTypeDescriptorBuilder(RawTypeDescriptorBuilder&&)                 = default;
            RawTypeDescriptorBuilder& operator=(RawTypeDescriptorBuilder&&)      = default;

            template<typename C, typename T>
            void AddField(const std::string& name, T C::*var)
            {
                FieldAccessor field {var};
                field.m_name = name;
                m_type_descriptor->m_fields.push_back(std::move(field));
            }

            template<typename FUNC>
            void AddMethod(const std::string& name, FUNC func)
            {
                MethodAccessor method {func};
                method.m_name = name;
                m_type_descriptor->m_methods.push_back(std::move(method));
            }

        private:
            std::unique_ptr<TypeDescriptor> m_type_descriptor {nullptr};
        };

        template<typename T>
        class TypeDescriptorBuilder
        {
        public:
            explicit TypeDescriptorBuilder(const std::string& name)
                : m_raw_builder(name)
            {}

            template<typename V>
            TypeDescriptorBuilder& AddField(const std::string& name, V T::*var)
            {
                m_raw_builder.AddField(name, var);
                return *this;
            }

            template<typename FUNC>
            TypeDescriptorBuilder& AddMethod(const std::string& name, FUNC func)
            {
                m_raw_builder.AddMethod(name, func);
                return *this;
            }

        private:
            RawTypeDescriptorBuilder m_raw_builder;
        };

        template<typename T>
        TypeDescriptorBuilder<T> AddClass(const std::string& name)
        {
            return TypeDescriptorBuilder<T> {name};
        }
    } // namespace reflect

} // namespace Meow
