/*
 * SUGGESTION FROM THE CC TEAM:
 * double check the order of actions that are fired.
 * You can do this in (at least) two ways:
 * 1) by using gdb adding breakpoints to actions
 * 2) by adding printing statements in each action
 *
 * For 2), we suggest writing the code to make it straightforward to enable/disable all of them 
 * (e.g., assuming shouldIPrint is a global variable
 *    if (shouldIPrint) std::cerr << "MY OUTPUT" << std::endl;
 * )
 */
#include <sched.h>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <set>
#include <iterator>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <stdint.h>
#include <assert.h>

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>

#include <L3.h>
#include <parser.h>
#include <helper.h> 
#include <tree.h>

static constexpr bool PARSER_DEBUG = false;

#define PARSER_PRINT(msg) \
  do { if (PARSER_DEBUG) std::cerr << msg << std::endl; } while (0)


namespace pegtl = TAO_PEGTL_NAMESPACE;

using namespace pegtl;

namespace L3 {

  /* 
   * Tokens parsed
   */ 
  std::vector<Item *> parsed_items; 

  Function* current_function = nullptr; 
  bool parsing_params = false; 
  size_t args_begin = 0; 

  OP last_op; 
  CMP last_cmp; 
  CallType last_call_type;

  /* 
   * Grammar rules from now on.
   */

  

  // Keywords

  struct str_arrow : TAO_PEGTL_STRING( "<-" ) {};
  struct str_var_left_paren  : TAO_PEGTL_STRING("(") {};
  struct str_var_right_paren : TAO_PEGTL_STRING(")") {};
  struct str_arg_left_paren  : TAO_PEGTL_STRING("(") {};
  struct str_arg_right_paren : TAO_PEGTL_STRING(")") {};
  struct str_left_brace  : TAO_PEGTL_STRING("{") {};
  struct str_right_brace : TAO_PEGTL_STRING("}") {};
  struct str_comma       : TAO_PEGTL_STRING(",") {};


  // Arith op keywords 

  struct str_plus : TAO_PEGTL_STRING( "+" ) {};
  struct str_minus : TAO_PEGTL_STRING( "-" ) {};
  struct str_times : TAO_PEGTL_STRING( "*" ) {};
  struct str_at : TAO_PEGTL_STRING( "&" ) {};
  struct str_left_shift : TAO_PEGTL_STRING( "<<" ) {};
  struct str_right_shift : TAO_PEGTL_STRING( ">>" ) {};


  // Comp op 

  struct str_less_than : TAO_PEGTL_STRING( "<" ) {}; 
  struct str_less_than_equal : TAO_PEGTL_STRING( "<=" ) {}; 
  struct str_equal : TAO_PEGTL_STRING( "=" ) {}; 
  struct str_greater_than : TAO_PEGTL_STRING( ">" ) {};
  struct str_greater_than_equal : TAO_PEGTL_STRING( ">=" ) {};  

  // Load and store keywords 

  struct str_load : TAO_PEGTL_STRING( "load" ) {};
  struct str_store : TAO_PEGTL_STRING( "store" ) {};

  // Break keyword 

  struct str_break : TAO_PEGTL_STRING( "br" ) {}; 

  // Function keywords 

  struct str_define : TAO_PEGTL_STRING( "define" ) {};
  struct str_call : TAO_PEGTL_STRING( "call" ) {};
  struct str_print : TAO_PEGTL_STRING( "print" ) {};
  struct str_input : TAO_PEGTL_STRING( "input" ) {};
  struct str_allocate : TAO_PEGTL_STRING( "allocate" ) {};
  struct str_tuple_error : TAO_PEGTL_STRING( "tuple-error" ) {};
  struct str_tensor_error : TAO_PEGTL_STRING( "tensor-error" ) {};
  struct str_return : TAO_PEGTL_STRING( "return" ) {};

  struct op_rule:
    pegtl::sor<
      str_plus, 
      str_minus, 
      str_times, 
      str_at, 
      str_left_shift, 
      str_right_shift
    > {}; 
  

  struct cmp_rule:
    pegtl::sor<
      str_less_than_equal,
      str_less_than,  
      str_equal,  
      str_greater_than_equal,
      str_greater_than
    > {}; 


  struct name:
    pegtl::seq<
      pegtl::plus< 
        pegtl::sor<
          pegtl::alpha,
          pegtl::one< '_' >
        >
      >,
      pegtl::star<
        pegtl::sor<
          pegtl::alpha,
          pegtl::one< '_' >,
          pegtl::digit
        >
      >
    > {};


  struct variable_rule:
    pegtl::seq<
      pegtl::one< '%' >, 
      name
    > {}; 


  struct label:
    pegtl::seq<
      pegtl::one<':'>,
      name
    > {};
  
  struct label_piece:
    pegtl::seq<
      pegtl::one<':'>,
      name
    > {};

  struct function_name_rule:
    pegtl::seq<
      pegtl::one<'@'>,
      name
    > {};

  struct function_name_piece_rule:
    pegtl::seq<
      pegtl::one<'@'>,
      name
    > {};

  struct number:
    pegtl::seq<
      pegtl::opt<
        pegtl::sor<
          pegtl::one< '-' >,
          pegtl::one< '+' >
        >
      >,
      pegtl::plus< 
        pegtl::digit
      >
    >{};

  struct u_rule: 
    pegtl::sor<
      variable_rule, 
      function_name_piece_rule
    > {};

  struct t_rule: 
    pegtl::sor<
      variable_rule, 
      number
    > {};

  struct s_rule: 
    pegtl::sor<
      t_rule, 
      label_piece, 
      function_name_piece_rule
    > {};

  /*
   * Separators.
   */

  struct spaces :
    pegtl::star< 
      pegtl::sor<
        pegtl::one< ' ' >,
        pegtl::one< '\t'>
      >
    > { };

  struct seps : 
    pegtl::star<
      pegtl::seq<
        spaces,
        pegtl::eol
      >
    > { };


  struct comment :
    pegtl::disable<
      pegtl::seq< TAO_PEGTL_STRING("//"), pegtl::until< pegtl::eolf > >
    > {};


  struct seps_with_comments : 
    pegtl::star< 
      pegtl::seq<
        spaces,
        pegtl::sor<
          pegtl::eol,
          comment
        >
      >
    > { };

  struct args_rule:
    pegtl::opt<
      pegtl::seq<
        t_rule,
        pegtl::star<
          spaces, str_comma, spaces, t_rule
        >
      >
    > {};
  
  struct vars_rule:
    pegtl::opt<
      pegtl::seq<
        variable_rule,
        pegtl::star<
          spaces, str_comma, spaces, variable_rule
        >
      >
    > {};

  struct callee_rule: 
    pegtl::sor<
      u_rule, 
      str_print, 
      str_allocate, 
      str_input, 
      str_tuple_error, 
      str_tensor_error
    > {}; 



  // Instruction rules 

  struct Instruction_assignment_rule:
    pegtl::seq<
      variable_rule, 
      spaces, 
      str_arrow, 
      spaces, 
      s_rule
    > {};

  struct Instruction_op_rule:
    pegtl::seq<
      variable_rule, 
      spaces, 
      str_arrow, 
      spaces, 
      t_rule, 
      spaces, 
      op_rule, 
      spaces, 
      t_rule
    > {};

  struct Instruction_cmp_rule:
    pegtl::seq<
      variable_rule, 
      spaces, 
      str_arrow, 
      spaces, 
      t_rule, 
      spaces, 
      cmp_rule, 
      spaces, 
      t_rule
    > {};

  struct Instruction_load_rule:
    pegtl::seq<
      variable_rule, 
      spaces, 
      str_arrow,
      spaces,
      str_load, 
      spaces, 
      variable_rule
    > {}; 

  struct Instruction_store_rule:
    pegtl::seq<
      str_store, 
      spaces, 
      variable_rule, 
      spaces, 
      str_arrow, 
      spaces, 
      s_rule
    > {}; 

  struct Instruction_return_rule:
    pegtl::seq<
      str_return
    > {};
  
  struct Instruction_return_t_rule:
    pegtl::seq<
      str_return, 
      spaces, 
      t_rule
    > {}; 


  struct Instruction_label_rule:
    pegtl::seq<
      label_piece
    >  {};


  struct Instruction_break_label_rule:
    pegtl::seq<
      str_break, 
      spaces, 
      label_piece
    > {}; 
  
  struct Instruction_break_t_label_rule:
    pegtl::seq<
      str_break, 
      spaces,
      t_rule,
      spaces,
      label_piece
    > {}; 

  struct Instruction_call_rule:
    pegtl::seq<
      str_call, 
      spaces, 
      callee_rule, 
      spaces, 
      str_arg_left_paren, 
      spaces, 
      args_rule, 
      spaces, 
      str_arg_right_paren
    > {};

  struct Instruction_call_assignment_rule:
    pegtl::seq<
      variable_rule, 
      spaces, 
      str_arrow, 
      spaces, 
      str_call, 
      spaces, 
      callee_rule, 
      spaces, 
      str_arg_left_paren, 
      spaces, 
      args_rule, 
      spaces, 
      str_arg_right_paren
    > {};


struct Instruction_rule:
  pegtl::sor<
    pegtl::seq< pegtl::at< Instruction_op_rule >      , Instruction_op_rule       >,
    pegtl::seq< pegtl::at< Instruction_cmp_rule >          , Instruction_cmp_rule           >,

    pegtl::seq< pegtl::at< Instruction_assignment_rule >              , Instruction_assignment_rule               >,

    pegtl::seq< pegtl::at< Instruction_load_rule > , Instruction_load_rule>,
    pegtl::seq< pegtl::at< Instruction_store_rule >               , Instruction_store_rule                >,

    pegtl::seq< pegtl::at< Instruction_return_t_rule >               , Instruction_return_t_rule                >,
    pegtl::seq< pegtl::at< Instruction_return_rule >                , Instruction_return_rule                 >,

    pegtl::seq< pegtl::at< Instruction_label_rule >         , Instruction_label_rule          >,
    pegtl::seq< pegtl::at< Instruction_break_label_rule >        , Instruction_break_label_rule         >,
    pegtl::seq< pegtl::at< Instruction_break_t_label_rule >           , Instruction_break_t_label_rule            >,

    pegtl::seq< pegtl::at< Instruction_call_assignment_rule >                 , Instruction_call_assignment_rule                  >,
    pegtl::seq< pegtl::at< Instruction_call_rule >           , Instruction_call_rule            >
  > {};


  struct Instructions_rule:
    pegtl::plus<
      pegtl::seq<
        seps_with_comments,
        pegtl::bol,
        spaces,
        Instruction_rule,
        spaces,
        pegtl::opt< comment >, 
        seps_with_comments
      >
    > { };

  struct Function_rule:
    pegtl::seq<
      seps_with_comments,
      pegtl::bol,
      spaces,
      str_define,
      spaces,
      function_name_rule,   
      spaces,
      str_var_left_paren,
      spaces,
      vars_rule,             
      spaces,
      str_var_right_paren,
      seps_with_comments,
      spaces,
      str_left_brace,
      seps_with_comments,
      Instructions_rule,     
      seps_with_comments,
      spaces,
      str_right_brace
    > {};


  struct Program_rule:
    pegtl::seq<
      seps_with_comments,
      pegtl::plus< Function_rule >,
      seps_with_comments,
      pegtl::eof
    > {};


  struct grammar :
    pegtl::must< Program_rule > 
    {};


  /* 
   * Actions attached to grammar rules.
   */

  template< typename Rule >
  struct action : pegtl::nothing< Rule > {};


  // Function rule 
  template<> struct action< str_define > {
    template<typename Input>
    static void apply(const Input&, Program& p) {
      current_function = new Function();
      p.functions.push_back(current_function);
    }
  };
    
  template<> struct action< function_name_rule > {
    template<typename Input>
    static void apply(const Input& in, Program& p) {
      current_function->name = in.string();
    }
  };

  template<> struct action< str_var_left_paren > {
    template<typename Input>
    static void apply(const Input& in, Program& p) {
      parsing_params = true; 
    }
  };
  
  template<> struct action< str_var_right_paren > {
    template<typename Input>
    static void apply(const Input& in, Program& p) {
      parsing_params = false; 
    }
  };


  template<> struct action< str_arg_left_paren > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      args_begin = parsed_items.size();
  }
};

  // Single push actions 

  // Push variable
  template<> struct action<variable_rule> {
    template<typename Input>
    static void apply(const Input& in, Program& p) { 
      auto v = new Variable(in.string());  

      parsed_items.push_back(v); 

      if (parsing_params && current_function) {
        current_function->var_arguments.push_back(v);
      } 
    }
  };


  // Push a number 
  template<> struct action < number > {
    template< typename Input >
    static void apply (const Input &in, Program &p) {
      auto n = new Number(std::stoll(in.string()));      
      parsed_items.push_back(n);
    }
  };

  // Push a label 
  template<> struct action < label_piece > {
    template< typename Input >
    static void apply (const Input &in, Program &p) {
      auto l = new Label(in.string());
      parsed_items.push_back(l);
    }
  };

  // Push a function name piece
  template<> struct action < function_name_piece_rule > {
    template< typename Input >
    static void apply (const Input &in, Program &p) {
      auto f = new Func(in.string());
      parsed_items.push_back(f);
    }
  };

  // Push arith ops 
  template<> struct action < op_rule > {
    template< typename Input >
    static void apply (const Input &in, Program &p) {
      last_op = op_from_string(in.string()); 
    }
  };


  // Push comp ops 

  template<> struct action < cmp_rule > {
    template< typename Input >
    static void apply (const Input &in, Program &p) {
      last_cmp = cmp_from_string(in.string()); 
    }
  };

  // Push call type 
  template<> struct action<u_rule> {
    template<typename Input>
    static void apply(const Input&, Program&) {
      last_call_type = CallType::l3; 
    }
  };

  template<> struct action< str_print > {
    template<typename Input>
    static void apply(const Input&, Program&) { last_call_type = CallType::print; }
  };

  template<> struct action< str_input > {
    template<typename Input>
    static void apply(const Input&, Program&) { last_call_type = CallType::input; }
  };

  template<> struct action< str_allocate > {
    template<typename Input>
    static void apply(const Input&, Program&) { last_call_type = CallType::allocate; }
  };

  template<> struct action< str_tuple_error > {
    template<typename Input>
    static void apply(const Input&, Program&) { last_call_type = CallType::tuple_error; }
  };

  template<> struct action< str_tensor_error > {
    template<typename Input>
    static void apply(const Input&, Program&) { last_call_type = CallType::tensor_error; }
  };


// Actions to build instruction nodes 

template<> struct action< Instruction_assignment_rule > {
  template<typename Input>
  static void apply(const Input&, Program&) {
    auto* currentF = current_function;

    Item* src = parsed_items.back(); parsed_items.pop_back();
    auto* dst = static_cast<Variable*>(parsed_items.back()); parsed_items.pop_back();

    currentF->instructions.push_back(new Instruction_assignment(dst, src));

    PARSER_PRINT("Assignment instruction");
  }
};

template<> struct action< Instruction_op_rule > {
  template<typename Input>
  static void apply(const Input&, Program&) {
    auto* currentF = current_function;

    Item* rhs = parsed_items.back(); parsed_items.pop_back();
    Item* lhs = parsed_items.back(); parsed_items.pop_back();
    auto* dst = static_cast<Variable*>(parsed_items.back()); parsed_items.pop_back();

    currentF->instructions.push_back(new Instruction_op(dst, lhs, last_op, rhs));
        PARSER_PRINT("Op instruction");
  }
};

template<> struct action< Instruction_cmp_rule > {
  template<typename Input>
  static void apply(const Input&, Program&) {
    auto* currentF = current_function;

    Item* rhs = parsed_items.back(); parsed_items.pop_back();
    Item* lhs = parsed_items.back(); parsed_items.pop_back();
    auto* dst = static_cast<Variable*>(parsed_items.back()); parsed_items.pop_back();

    currentF->instructions.push_back(new Instruction_cmp(dst, lhs, last_cmp, rhs));
        PARSER_PRINT("Cmp instruction");
  }
};

template<> struct action< Instruction_load_rule > {
  template<typename Input>
  static void apply(const Input&, Program&) {
    auto* currentF = current_function;

    auto* src = static_cast<Variable*>(parsed_items.back()); parsed_items.pop_back();
    auto* dst = static_cast<Variable*>(parsed_items.back()); parsed_items.pop_back();

    currentF->instructions.push_back(new Instruction_load(dst, src));
        PARSER_PRINT("Load instruction");
  }
};

template<> struct action< Instruction_store_rule > {
  template<typename Input>
  static void apply(const Input&, Program&) {
    auto* currentF = current_function;

    Item* src = parsed_items.back(); parsed_items.pop_back();
    auto* dst = static_cast<Variable*>(parsed_items.back()); parsed_items.pop_back();

    currentF->instructions.push_back(new Instruction_store(dst, src));
        PARSER_PRINT("Store instruction");
  }
};

template<> struct action< Instruction_return_rule > {
  template<typename Input>
  static void apply(const Input&, Program&) {
    current_function->instructions.push_back(new Instruction_return());
        PARSER_PRINT("Return instruction");
  }
};

template<> struct action< Instruction_return_t_rule > {
  template<typename Input>
  static void apply(const Input&, Program&) {
    Item* ret = parsed_items.back(); parsed_items.pop_back();
    current_function->instructions.push_back(new Instruction_return_t(ret));
        PARSER_PRINT("Return t instruction");
  }
};

template<> struct action< Instruction_label_rule > {
  template<typename Input>
  static void apply(const Input&, Program&) {
    auto* currentF = current_function;

    auto* l = static_cast<Label*>(parsed_items.back()); parsed_items.pop_back();
    currentF->instructions.push_back(new Instruction_label(l));
        PARSER_PRINT("Label instruction");
  }
};

template<> struct action< Instruction_break_label_rule > {
  template<typename Input>
  static void apply(const Input&, Program&) {
    auto* currentF = current_function;

    auto* l = static_cast<Label*>(parsed_items.back()); parsed_items.pop_back();
    currentF->instructions.push_back(new Instruction_break_label(l));
        PARSER_PRINT("Break label instruction");
  }
};

template<> struct action< Instruction_break_t_label_rule > {
  template<typename Input>
  static void apply(const Input&, Program&) {
    auto* currentF = current_function;

    auto* l = static_cast<Label*>(parsed_items.back()); parsed_items.pop_back();
    Item* t = parsed_items.back(); parsed_items.pop_back();

    currentF->instructions.push_back(new Instruction_break_t_label(t, l));
        PARSER_PRINT("Break t label instruction");
  }
};

template<> struct action< Instruction_call_rule > {
  template<typename Input>
  static void apply(const Input&, Program&) {
    auto* currentF = current_function;

    // args are parsed_items[callee (if l3 calltype), args_begin..end)

    std::vector<Item*> args;
    for (size_t i = args_begin; i < parsed_items.size(); ++i) args.push_back(parsed_items[i]);
    parsed_items.resize(args_begin);

    Item* callee = nullptr;
    if (last_call_type == CallType::l3) {
      callee = parsed_items.back(); parsed_items.pop_back();
    }
            PARSER_PRINT(args.size());
    currentF->instructions.push_back(new Instruction_call(last_call_type, callee, std::move(args)));
    PARSER_PRINT("Call instruction");
  }
};

template<> struct action< Instruction_call_assignment_rule > {
  template<typename Input>
  static void apply(const Input&, Program&) {
    auto* currentF = current_function;

    std::vector<Item*> args;
    for (size_t i = args_begin; i < parsed_items.size(); ++i) args.push_back(parsed_items[i]);
    parsed_items.resize(args_begin);

    Item* callee = nullptr;
    if (last_call_type == CallType::l3) { 
      callee = parsed_items.back(); parsed_items.pop_back();
    }
    auto* dst = static_cast<Variable*>(parsed_items.back()); parsed_items.pop_back();

              PARSER_PRINT(args.size());
    currentF->instructions.push_back(new Instruction_call_assignment(dst, last_call_type, callee, std::move(args)));
        PARSER_PRINT("Call assignment instruction");
  }
};


  Program parse_file (char *fileName){

    /* 
     * Check the grammar for some possible issues.
     */
    if (pegtl::analyze< grammar >() != 0){
      std::cerr << "There are problems with the grammar" << std::endl;
      exit(1);
    }

    /*
     * Parse.
     */
    file_input< > fileInput(fileName);
    Program p;
    parse< grammar, action >(fileInput, p);

    return p;
  }
}
