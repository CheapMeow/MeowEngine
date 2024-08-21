#pragma once

#include "reflect.hpp"

namespace Meow
{
    namespace reflect
    {

        template<typename ClassType>
        class TypeDescriptorBuilder
        {
        public:
            explicit TypeDescriptorBuilder(const std::string& name)
                : m_type_descriptor(name)
            {}

            ~TypeDescriptorBuilder() { Registry::instance().Register(std::move(m_type_descriptor)); }

            TypeDescriptorBuilder(const TypeDescriptorBuilder&)            = delete;
            TypeDescriptorBuilder& operator=(const TypeDescriptorBuilder&) = delete;
            TypeDescriptorBuilder(TypeDescriptorBuilder&&)                 = default;
            TypeDescriptorBuilder& operator=(TypeDescriptorBuilder&&)      = default;

            template<typename FieldType>
            TypeDescriptorBuilder&
            AddField(const std::string& name, const std::string& type_name, FieldType ClassType::*field_ptr)
            {
                m_type_descriptor.AddField({name, type_name, field_ptr});
                return *this;
            }

            template<typename MethodType>
            TypeDescriptorBuilder& AddMethod(const std::string& name, MethodType method)
            {
                m_type_descriptor.AddMethod({name, method});
                return *this;
            }

        private:
            TypeDescriptor m_type_descriptor;
        };

        template<typename ClassType>
        TypeDescriptorBuilder<ClassType> AddClass(const std::string& name)
        {
            return TypeDescriptorBuilder<ClassType> {name};
        }
    } // namespace reflect

} // namespace Meow
