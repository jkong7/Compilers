#pragma once 

#include <algorithm> 
#include <iterator> 
#include <unordered_map> 
#include <unordered_set> 
#include <vector> 
#include <sstream> 
#include <tuple> 
#include <L3.h>


namespace L3{

  class ContextBehavior : public Behavior {
  public:
    ContextBehavior() = default;

    void act(Program& p) override;
    void act(Function& f) override;

    void act(Instruction_assignment& i) override;
    void act(Instruction_op& i) override;
    void act(Instruction_cmp& i) override;
    void act(Instruction_load& i) override;
    void act(Instruction_store& i) override;
    void act(Instruction_return& i) override;
    void act(Instruction_return_t& i) override;

    void act(Instruction_label& i) override;
    
 

    void act(Instruction_call& i) override;
    void act(Instruction_call_assignment& i) override;

  private:
    void end_context(); 

    std::vector<Context> contexts; 
    Instruction* cur_instruction; 
    size_t cur_context = 0; 

  };
}