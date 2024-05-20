#include "reflect.hpp"

#include <iostream>

namespace Meow
{
    namespace reflect
    {
        TypeDescriptor* Registry::Find(const std::string& name)
        {
            return m_type_descriptor_map.find(name)->second.get();
        }

        void Registry::Register(std::unique_ptr<TypeDescriptor> desc)
        {
            if (desc == nullptr)
                return;

            auto name                   = desc->name();
            m_type_descriptor_map[name] = std::move(desc);
        }

        void Registry::Clear()
        {
            decltype(m_type_descriptor_map) tmp;
            tmp.swap(m_type_descriptor_map);
        }

        TypeDescriptor& GetByName(const std::string& name) { return *Registry::instance().Find(name); }

        void ClearRegistry() { Registry::instance().Clear(); }

    } // namespace reflect
} // namespace Meow
