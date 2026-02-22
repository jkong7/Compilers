  #include <helper.h> 

  namespace IR {
    Type type_from_string(std::string_view s) {
        return s == "tuple" ? Type::tuple :
            s == "code" ? Type::code :
            s == "void" ? Type::void_ : 
            s == "int64" ? Type::int64 :
            throw std::runtime_error("bad Type");
    }

    OP op_from_string(std::string_view s) {
    return s == "+" ? OP::plus :
          s == "-"  ? OP::minus :
          s == "*"  ? OP::times :
          s == "&"  ? OP::at :
          s == "<<" ? OP::left_shift :
          s == ">>" ? OP::right_shift :
          s == "<"  ? OP::less_than :
          s == "<=" ? OP::less_than_equal :
          s == "="  ? OP::equal :
          s == ">"  ? OP::greater_than :
          s == ">=" ? OP::greater_than_equal :
          throw std::runtime_error("bad OP");
  }

  }