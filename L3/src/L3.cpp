#include <sstream> 

#include <L3.h>
#include <helper.h> 

namespace L3 {


// Items 

Number::Number (int64_t n)
  : number_ {n}{
    return ; 
  }


Label::Label (const std::string &s)
  : label_ {s} {
    return; 
  }

Func::Func (const std::string &s)
  : function_label_ {s} {
    return; 
  }

Variable::Variable(const std::string &s)
  : var_ (s) {
    return; 
  }



ItemType Number::kind() const {
  return ItemType::NumberItem; 
}

ItemType Label::kind() const {
  return ItemType::LabelItem; 
}

ItemType Func::kind() const {
  return ItemType::FuncItem; 
}

ItemType Variable::kind() const {
  return ItemType::VariableItem; 
}


std::string Number::emit(const EmitOptions& options) const {
  return std::to_string(number_); 
}

std::string Label::emit(const EmitOptions& options) const {
  if (options.l2tol1) {
    return label_; 
  }
  std::string lname = label_.substr(1); 
  std::ostringstream s; 
  std::string prefix = options.memoryStoredLabel ? "$_" : "_"; 
  s << prefix << lname;
  return s.str(); 
}

std::string Func::emit(const EmitOptions& options) const {
  if (options.l2tol1) {
    return function_label_; 
  }
  std::string fname = function_label_.substr(1); 
  std::ostringstream s; 
  if (options.functionCall) {
    s << "_" << fname; 
  } else if (options.l3tol2) {
    s << "@" << fname; 
  } else {
    s << "$_" << fname; 
  }
  return s.str(); 
}

std::string Variable::emit(const EmitOptions& options) const {
  return var_; 
}



// Instruction constructors 
Instruction_assignment::Instruction_assignment(Variable* dst, Item* src)
  : dst_{dst},
    src_{src} {
  return;
}

Instruction_op::Instruction_op(Variable* dst, Item* lhs, OP op, Item* rhs)
  : dst_{dst},
    lhs_{lhs},
    op_{op},
    rhs_{rhs} {
  return;
}

Instruction_cmp::Instruction_cmp(Variable* dst, Item* lhs, CMP cmp, Item* rhs)
  : dst_{dst},
    lhs_{lhs},
    cmp_{cmp},
    rhs_{rhs} {
  return;
}

Instruction_load::Instruction_load(Variable* dst, Variable* src)
  : dst_{dst},
    src_{src} {
  return;
}

Instruction_store::Instruction_store(Variable* dst, Item* src)
  : dst_{dst},
    src_{src} {
  return;
}

Instruction_return::Instruction_return() {
  return;
}

Instruction_return_t::Instruction_return_t(Item* ret)
  : ret_{ret} {
  return;
}

Instruction_label::Instruction_label(Label* l)
  : label_{l} {
  return;
}

Instruction_break_label::Instruction_break_label(Label* l)
  : label_{l} {
  return;
}

Instruction_break_t_label::Instruction_break_t_label(Item* t, Label* l)
  : t_{t},
    label_{l} {
  return;
}

Instruction_call::Instruction_call(CallType c, Item* callee, std::vector<Item*> args)
  : c_{c},
    callee_{callee},
    args_{std::move(args)} {
  return;
}

Instruction_call_assignment::Instruction_call_assignment(Variable* dst, CallType c, Item* callee, std::vector<Item*> args)
  : dst_{dst},
    c_{c},
    callee_{callee},
    args_{std::move(args)} {
  return;
}



// Visitor 

void Program::accept(Behavior& b) {
  b.act(*this);
}

void Function::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_assignment::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_op::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_cmp::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_load::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_store::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_return::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_return_t::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_label::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_break_label::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_break_t_label::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_call::accept(Behavior& b) {
  b.act(*this);
}

void Instruction_call_assignment::accept(Behavior& b) {
  b.act(*this);
}


}

