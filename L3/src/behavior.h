#pragma once



namespace L3 {

    class Program;
    class Function;

    class Instruction_assignment;
    class Instruction_op;
    class Instruction_cmp;
    class Instruction_load;
    class Instruction_store;
    class Instruction_return;
    class Instruction_return_t;
    class Instruction_label;
    class Instruction_break_label;
    class Instruction_break_t_label;
    class Instruction_call;
    class Instruction_call_assignment;

  class Behavior {
    public:
      virtual ~Behavior() = default;

      virtual void act(Program &p) = 0;
      virtual void act(Function &f) = 0;

      virtual void act(Instruction_assignment &i) = 0;
      virtual void act(Instruction_op &i) = 0;
      virtual void act(Instruction_cmp &i) = 0;
      virtual void act(Instruction_load &i) = 0;
      virtual void act(Instruction_store &i) = 0;

      virtual void act(Instruction_return &i) = 0;
      virtual void act(Instruction_return_t &i) = 0;

      virtual void act(Instruction_label &i) = 0;
      virtual void act(Instruction_break_label &i) = 0;
      virtual void act(Instruction_break_t_label &i) = 0;

      virtual void act(Instruction_call &i) = 0;
      virtual void act(Instruction_call_assignment &i) = 0;
  };

} 
