#pragma once 

#include <algorithm> 
#include <iterator> 
#include <unordered_map> 
#include <unordered_set> 
#include <vector> 
#include <sstream> 
#include <tuple> 
#include <IR.h>
#include <behavior.h> 


namespace IR{

  class CodeGenBehavior : public Behavior {
  public:
    CodeGenBehavior() = default;

    void act(Program& p) override;
    void act(Function& f) override;

    void act(Instruction_initialize& i) override;
    void act(Instruction_assignment& i) override;
    void act(Instruction_op& i) override;

    void act(Instruction_index_load& i) override;
    void act(Instruction_index_store& i) override;

    void act(Instruction_length& i) override;
    void act(Instruction_length_t& i) override;

    void act(Instruction_call& i) override;
    void act(Instruction_call_assignment& i) override;

    void act(Instruction_new_array& i) override;
    void act(Instruction_new_tuple& i) override;

    void act(Instruction_break_uncond& i) override;
    void act(Instruction_break_cond& i) override;

    void act(Instruction_return& i) override;
    void act(Instruction_return_t& i) override;
  };

}