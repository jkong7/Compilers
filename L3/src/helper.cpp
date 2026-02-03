#include <helper.h>

namespace L3 {
  OP op_from_string(std::string_view s) {
    return s == "+" ? OP::plus :
          s == "-"  ? OP::minus :
          s == "*"  ? OP::times :
          s == "&"  ? OP::at :
          s == "<<" ? OP::left_shift :
          s == ">>" ? OP::right_shift :
          throw std::runtime_error("bad OP");
  }

  CMP cmp_from_string(std::string_view s) {
    return s == "<" ? CMP::less_than :
          s == "<=" ? CMP::less_than_equal :
          s == "="  ? CMP::equal :
          s == ">"  ? CMP::greater_than :
          s == ">=" ? CMP::greater_than_equal :
          throw std::runtime_error("bad CMP");
  }
}
