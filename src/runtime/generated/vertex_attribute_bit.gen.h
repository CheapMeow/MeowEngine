#pragma once  
  
#include <string>  
  
namespace Meow  
{  
    enum class VertexAttributeBit;  
  
    VertexAttributeBit to_enum(const std::string& str);  
  
    const std::string to_string(VertexAttributeBit enum_val);  
} // namespace Meow  
