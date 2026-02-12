#pragma once

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <variant>
#include <iostream>
#include <memory> 

#include <behavior.h> 



namespace L3 {
  struct Tree; 
  struct Behavior; 

  // Enums 

  // Consider making enum classes

  enum OP {plus, minus, times, at, left_shift, right_shift}; 

  enum CMP {less_than, less_than_equal, equal, greater_than_equal, greater_than}; 

  enum CallType {l3, print, input, allocate, tuple_error, tensor_error}; 



  // Items 

  enum ItemType { NumberItem, LabelItem, FuncItem, VariableItem }; 

  struct EmitOptions {
    bool l2tol1 = false; 
    bool eightBitRegister = false; 
    bool memoryStoredLabel = false; 
    bool functionCall = false; 
    bool indirectRegCall = false; 
    bool livenessAnalysis = false; 
    bool l3tol2 = false; 

    const std::unordered_map<std::string, std::string>* coloring = nullptr; 
  }; 

  class Item {
    public: 
      virtual ~Item() = default; 
      virtual std::string emit(const EmitOptions& options = EmitOptions{}) const = 0; 
      virtual ItemType kind() const = 0; 
  };


  class Number : public Item {
    public:
      Number (int64_t n); 
      std::string emit(const EmitOptions& options = EmitOptions{}) const override;
      ItemType kind() const override; 

      int64_t number_; 

  }; 

  class Label : public Item {
    public: 
      Label (const std::string &s); 
      std::string emit(const EmitOptions& options = EmitOptions{}) const override;
      ItemType kind() const override; 

      std::string label_; 
  }; 

  class Func : public Item {
    public: 
      Func (const std::string &s); 
      std::string emit(const EmitOptions& options = EmitOptions{}) const override;
      ItemType kind() const override; 

      std::string function_label_; 
  }; 

  class Variable : public Item {
    public: 
      Variable (const std::string &s); 
      std::string emit(const EmitOptions& options = EmitOptions{}) const override; 
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

  using Node = std::variant<std::unique_ptr<Tree>, Instruction_label*, Instruction_call*, Instruction_call_assignment*>; 

  struct Context {
    std::vector<Node> trees; 
  };

  /*
   * Instructions.
   */

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


  class Instruction_cmp : public Instruction {
  public:
    Instruction_cmp(Variable* dst, Item* lhs, CMP cmp, Item* rhs);
    void accept(Behavior& b) override;

    Variable* dst_; 
    Item* lhs_; 
    CMP cmp_; 
    Item* rhs_;
  };


  class Instruction_load : public Instruction {
  public:
    Instruction_load(Variable* dst, Variable* src);
    void accept(Behavior& b) override;

    Variable* dst_; 
    Variable* src_; 
  };


  class Instruction_store : public Instruction {
  public:
    Instruction_store(Variable* dst, Item* src);
    void accept(Behavior& b) override;

    Variable* dst_;
    Item* src_; 
  };


  class Instruction_return : public Instruction {
  public:
    Instruction_return();
    void accept(Behavior& b) override;
  };


  class Instruction_return_t : public Instruction {
  public:
    Instruction_return_t(Item* ret);
    void accept(Behavior& b) override;

    Item* ret_;
  };


  class Instruction_label : public Instruction {
  public:
    Instruction_label(Label* l);
    void accept(Behavior& b) override;

    Label* label_; 
  };


  class Instruction_break_label : public Instruction {
  public:
    Instruction_break_label(Label* l);
    void accept(Behavior& b) override;

    Label* label_;
  };


  class Instruction_break_t_label : public Instruction {
  public:
    Instruction_break_t_label(Item* t, Label* l); 
    void accept(Behavior& b) override;

    Item* t_; 
    Label* label_;
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


  class Function{
    public:
      std::string name;
      std::vector<Variable*> var_arguments;
      std::vector<Instruction *> instructions;
      std::vector<Context> contexts; 

      void accept(Behavior& b);

  };

  class Program{
    public:
      std::vector<Function *> functions;
      
      void accept(Behavior& b); 
  };

}
