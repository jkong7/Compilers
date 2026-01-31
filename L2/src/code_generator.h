#pragma once 


#include <L2.h> 
#include <behavior.h> 


namespace L2 {
  class CodeGenBehavior : public Behavior {
    public:
      explicit CodeGenBehavior(std::ofstream &out, const std::vector<std::unordered_map<std::string, std::string>> &colorInputs, const std::vector<size_t>);
      void act(Program &p) override; 
      void act(Function &f) override; 
      virtual void act(Instruction_assignment &i) override; 
      virtual void act(Instruction_stack_arg_assignment &i) override; 
      virtual void act(Instruction_aop &i) override; 
      virtual void act(Instruction_sop &i) override; 
      virtual void act(Instruction_mem_aop &i) override; 
      virtual void act(Instruction_cmp_assignment &i) override; 
      virtual void act(Instruction_cjump &i) override; 
      virtual void act(Instruction_label &i) override; 
      virtual void act(Instruction_goto &i) override; 
      virtual void act(Instruction_ret &i) override; 
      virtual void act(Instruction_call &i) override; 
      virtual void act(Instruction_reg_inc_dec &i) override; 
      virtual void act(Instruction_lea &i) override; 

    private: 
      size_t cur_f = 0; 
      int64_t arguments; 

      std::vector<std::unordered_map<std::string, std::string>> colorInputs; 
      std::vector<size_t> locals;  
      std::ofstream &out; 
  };

  void generate_code(Program &p, const std::vector<std::unordered_map<std::string, std::string>> &colorInputs, const std::vector<size_t>);
}
