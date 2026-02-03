#pragma once 

#include <string_view> 
#include <stdexcept>
#include <unordered_map> 
#include <unordered_set> 
#include <vector> 

#include <L3.h> 

namespace L3 {
    OP op_from_string(std::string_view s);
    CMP cmp_from_string(std::string_view s);
}
