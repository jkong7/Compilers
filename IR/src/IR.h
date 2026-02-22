#pragma once

#include <algorithm>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <variant>
#include <iostream>
#include <memory> 



namespace IR {
  struct Behavior; 

  // Enums 

  enum OP {plus, minus, times, at, left_shift, right_shift, less_than, less_than_equal, equal, greater_than_equal, greater_than}; 

  enum CallType {ir, print, input, tuple_error, tensor_error}; 

  enum Type {void_, tuple, code, int64}; 


  // Items 

  enum ItemType { NumberItem, LabelItem, FuncItem, VariableItem }; 

  class Item {
    public: 
      virtual ~Item() = default; 
      virtual std::string emit() const = 0; 
      virtual ItemType kind() const = 0; 
  };


  class Number : public Item {
    public:
      Number (int64_t n); 
      std::string emit() const override;
      ItemType kind() const override; 

      int64_t number_; 

  }; 

  class Label : public Item {
    public: 
      Label (const std::string &s); 
      std::string emit() const override;
      ItemType kind() const override; 

      std::string label_; 
  }; 

  class Func : public Item {
    public: 
      Func (const std::string &s); 
      std::string emit() const override;
      ItemType kind() const override; 

      std::string function_label_; 
  }; 

  class Variable : public Item {
    public: 
      Variable (const std::string &s); 
      std::string emit() const override; 
      ItemType kind() const override; 

      std::string var_; 
  }; 




  /*
   * Instruction interface.
   */
  class Instruction{
    public: 
      virtual ~Instruction() = default; 
      void virtual accept(Behavior& b) = 0; 
  };

  /*
   * Instructions.
   */

  class Instruction_initialize : public Instruction {
  public:
    Instruction_initialize(Variable* var);
    void accept(Behavior& b) override;

    Variable* var_; 
  };

  class Instruction_assignment : public Instruction {
    public:
      Instruction_assignment(Variable* dst, Item* src);
      void accept(Behavior& b) override; 

      Variable* dst_; 
      Item* src_; 
  }; 


  class Instruction_op : public Instruction {
  public:
    Instruction_op(Variable* dst, Item* lhs, OP op, Item* rhs);
    void accept(Behavior& b) override;

    Variable* dst_; 
    Item* lhs_; 
    OP op_; 
    Item* rhs_;
  };


  class Instruction_index_load : public Instruction {
  public:
    Instruction_index_load(Variable* dst, Variable* src, std::vector<Item*> indexes);
    void accept(Behavior& b) override;

    Variable* dst_; 
    Variable* src_; 
    std::vector<Item*> indexes_; 
  };


  class Instruction_index_store : public Instruction {
  public:
    Instruction_index_store(Variable* dst, std::vector<Item*> indexes, Item* src);
    void accept(Behavior& b) override;

    Variable* dst_;
    std::vector<Item*> indexes_; 
    Item* src_; 
  };


  class Instruction_length_t : public Instruction {
  public:
    Instruction_length_t(Variable* dst, Variable* src, Item* t);
    void accept(Behavior& b) override;

    Variable* dst_; 
    Variable* src_; 
    Item* t_; 
  };


  class Instruction_length: public Instruction {
  public:
    Instruction_length(Variable* dst, Variable* src);
    void accept(Behavior& b) override;

    Variable* dst_; 
    Variable* src_; 
  };



  class Instruction_call : public Instruction {
  public:
    Instruction_call(CallType c, Item* callee, std::vector<Item*> args);
    void accept(Behavior& b) override;

    CallType c_; 
    Item* callee_; 
    std::vector<Item*> args_; 
  };


  class Instruction_call_assignment : public Instruction {
  public:
    Instruction_call_assignment(Variable* dst, CallType c, Item* callee, std::vector<Item*> args);
    void accept(Behavior& b) override;

    Variable* dst_;
    CallType c_; 
    Item* callee_; 
    std::vector<Item*> args_; 
  };

  class Instruction_new_array : public Instruction {
  public: 
    Instruction_new_array(Variable* dst, std::vector<Item*> args);
    void accept(Behavior& b) override; 

    Variable* dst_; 
    std::vector<Item*> args_; 
  }; 

  class Instruction_new_tuple : public Instruction {
  public: 
    Instruction_new_tuple(Variable* dst, Item* t); 
    void accept(Behavior& b) override; 

    Variable* dst_; 
    Item* t_; 
  };

  class Instruction_break_uncond : public Instruction {
  public:
    Instruction_break_uncond(Label* label); 
    void accept(Behavior& b) override; 

    Label* label_; 
  }; 

  class Instruction_break_cond : public Instruction {
  public: 
    Instruction_break_cond(Item* t, Label* label1, Label* label2); 
    void accept(Behavior& b) override; 

    Item* t_; 
    Label* label1_; 
    Label* label2_; 
  }; 

  class Instruction_return : public Instruction {
  public:
    Instruction_return();
    void accept(Behavior& b) override; 
  };

  class Instruction_return_t : public Instruction {
  public: 
    Instruction_return_t(Item* t); 
    void accept(Behavior& b) override;

    Item* t_;
  };

  class BasicBlock {
    public:
      Label* label_; 
      std::vector<Instruction*> instructions; 
      std::vector<std::string> succ_labels; 
      std::vector<BasicBlock*> succs; 
  };


  class Function{
    public:
      Type return_type; 
      int64_t dims; 
      std::string name;
      std::vector<Variable*> var_arguments;
      std::vector<BasicBlock*> basic_blocks; 
      std::unordered_map<std::string, std::pair<Type, int64_t>> variable_types;
      std::unordered_map<std::string, BasicBlock*> label_to_bb; 

      void accept(Behavior& b);
      void fill_succs();

  };

  class Program{
    public:
      std::vector<Function *> functions;
      
      void accept(Behavior& b); 
      void linearize_bb(); 
  };

}
