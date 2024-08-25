#pragma once  

#include "core/reflect/macros.h"

#include <cstdint>
#include <string>  
  
namespace Meow  
{  
    enum class VertexAttributeBit : uint32_t;  
  
    VertexAttributeBit to_enum(const std::string& str);  
  
    const std::string to_string(VertexAttributeBit enum_val);  
} // namespace Meow  
