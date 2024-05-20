#include "type_descriptor_builder.hpp"

namespace Meow
{
    namespace reflect
    {
        RawTypeDescriptorBuilder::RawTypeDescriptorBuilder(const std::string& name)
            : m_type_descriptor(std::make_unique<TypeDescriptor>())
        {
            m_type_descriptor->m_name = name;
        }

        RawTypeDescriptorBuilder::~RawTypeDescriptorBuilder()
        {
            Registry::instance().Register(std::move(m_type_descriptor));
        }
    } // namespace reflect
} // namespace Meow