#include <string>
#include <iostream>
#include <fstream>

#include <code_generator.h>
#include <helper.h> 

using namespace std;

namespace L2{
  CodeGenBehavior::CodeGenBehavior(std::ofstream &out, const std::vector<std::unordered_map<std::string, std::string>> &colorInputs, const std::vector<size_t> locals)
    : out(out), colorInputs(colorInputs), locals(locals) {
      return; 
    }
 
  void CodeGenBehavior::act(Program &p) {
    out << "(" << p.entryPointLabel << "\n"; 
    for (Function* f: p.functions) {
      f->accept(*this);
      cur_f++; 
    }
    out << ")";
  }

  void CodeGenBehavior::act(Function& f) {
    arguments = f.arguments; 
    out << "  (" << f.name << "\n"; 
    out << f.arguments << " " << locals[cur_f] << "\n";
    for (Instruction* i: f.instructions) {
      i -> accept(*this); 
    }
    out << "  )\n";   
  }

  void CodeGenBehavior::act(Instruction_assignment &i) { // w <- s | w <- mem x M | mem x M <- s |

    EmitOptions options; 
    options.l2tol1 = true; 
    options.coloring = &colorInputs[cur_f]; 
    out << "  " << i.dst()->emit(options) << " <- " << i.src()->emit(options) << "\n";

  }

  void CodeGenBehavior::act(Instruction_stack_arg_assignment &i) { // offset = 8 * (stack_args - (M / 8))
    int64_t M = i.src()->value()->value();
    size_t offset = 8 * locals[cur_f] + M;

    EmitOptions options; 
    options.l2tol1 = true; 
    options.coloring = &colorInputs[cur_f];
    out << "  " << i.dst()->emit(options) << " <- " << "mem rsp " << offset << "\n";
  }

  void CodeGenBehavior::act(Instruction_aop &i) { // w aop t
    EmitOptions options; 
    options.l2tol1 = true; 
    options.coloring = &colorInputs[cur_f];
    out << "  " << i.dst()->emit(options) << " " << string_from_aop(i.aop()) << " " << i.rhs()->emit(options) << "\n"; 
  } 

  void CodeGenBehavior::act(Instruction_sop &i) {
    EmitOptions options; 
    options.l2tol1 = true; 
    options.coloring = &colorInputs[cur_f];
    out << "  " << i.dst()->emit(options) << " " << string_from_sop(i.sop()) << " " << i.src()->emit(options) << "\n";
  } 

  void CodeGenBehavior::act(Instruction_mem_aop &i) { // mem x M += t | mem x M -= t | w += mem x M | w -= mem x M |
    EmitOptions options; 
    options.l2tol1 = true; 
    options.coloring = &colorInputs[cur_f];
    out << "  " << i.lhs()->emit(options) << " " << string_from_aop(i.aop()) << " " << i.rhs()->emit(options) << "\n"; 
  } 

  void CodeGenBehavior::act(Instruction_cmp_assignment &i) {
    EmitOptions options; 
    options.l2tol1 = true; 
    options.coloring = &colorInputs[cur_f];
    out << "  " << i.dst()->emit(options) << " <- " << i.lhs()->emit(options) << " " << string_from_cmp(i.cmp()) << " " << i.rhs()->emit(options) << "\n"; 
  } 

  void CodeGenBehavior::act(Instruction_cjump &i) {
    EmitOptions options; 
    options.l2tol1 = true; 
    options.coloring = &colorInputs[cur_f];
    out << "  " << "cjump " << i.lhs()->emit(options) << " " << string_from_cmp(i.cmp()) << " " << i.rhs()->emit(options) << " " << i.label()->emit(options) << "\n"; 
  } 

  void CodeGenBehavior::act(Instruction_label &i) {
    EmitOptions options; 
    options.l2tol1 = true; 
    options.coloring = &colorInputs[cur_f];
    out << "  " << i.label()->emit(options) << "\n"; 
  } 

  void CodeGenBehavior::act(Instruction_goto &i) {
    EmitOptions options; 
    options.l2tol1 = true; 
    options.coloring = &colorInputs[cur_f];
    out << "  goto " << i.label()->emit(options) << "\n"; 
  } 

  void CodeGenBehavior::act(Instruction_ret &i) {
    out << "  return\n";
  } 

  void CodeGenBehavior::act(Instruction_call &i) {
    EmitOptions options; 
    options.l2tol1 = true; 
    options.coloring = &colorInputs[cur_f];
    if (i.callType() == l1) {
      out << "  call " << i.callee()->emit(options) << " " << i.nArgs()->emit(options) << "\n"; 
    } else if (i.callType() == print) {
      out << "  call print 1\n"; 
    } else if (i.callType() == allocate) {
      out << "  call allocate 2\n"; 
    } else if (i.callType() == input) {
      out << "  call input 0\n"; 
    } else if (i.callType() == tuple_error) {
      out << "  call tuple-error 3\n"; 
    } else if (i.callType() == tensor_error) {
      if (i.nArgs()->value() == 1) {
        out << "  call tensor-error 1\n"; 
      } else if (i.nArgs()->value() == 3) {
        out << "  call tensor-error 3\n"; 
      } else if (i.nArgs()->value() == 4) {
        out << "  call tensor-error 4\n";
      }
    }
  } 

  void CodeGenBehavior::act(Instruction_reg_inc_dec &i) {
    EmitOptions options; 
    options.l2tol1 = true; 
    options.coloring = &colorInputs[cur_f];
    out << "  " << i.dst()->emit(options) << " " << string_from_inc_dec(i.op()) << "\n"; 
  } 

  void CodeGenBehavior::act(Instruction_lea &i) {
    EmitOptions options; 
    options.l2tol1 = true; 
    options.coloring = &colorInputs[cur_f];
    out << "  " << i.dst()->emit(options) << " @ " << i.lhs()->emit(options) << " " << i.rhs()->emit(options) << i.scale()->emit(options) << "\n"; 
  } 


  void generate_code(Program &p, const std::vector<std::unordered_map<std::string, std::string>> &colorInputs, const std::vector<size_t> locals){

    std::ofstream outputFile;
    outputFile.open("prog.L1");

    // codegen
    CodeGenBehavior b(outputFile, colorInputs, locals);
    p.accept(b); 

    outputFile.close();
   
    return;
  }
}
