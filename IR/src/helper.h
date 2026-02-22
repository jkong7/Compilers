#pragma once 

#include <string_view> 
#include <stdexcept>

#include <IR.h> 

namespace IR {
    Type type_from_string(std::string_view s);
    OP op_from_string(std::string_view s); 
}
