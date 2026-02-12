#include "tiler.h"

#include <cassert>
#include <type_traits>
#include <variant>

namespace L3 {

  static bool is_leaf(const Tree& t) {
    return t.kind == TreeType::Leaf && t.leaf.has_value(); 
  }

  static const Tree* ptr(const std::unique_ptr<Tree>& p) {
    return p ? p.get() : nullptr; 
  }

  static std::string leaf_to_str(const Leaf& leaf) {
    return std::visit([](auto&& x) -> std::string {
      using T = std::decay_t<decltype(x)>;
      if constexpr (std::is_same_v<T, NumberLeaf>) return std::to_string(x.n);
      if constexpr (std::is_same_v<T, VarLeaf>)    return x.var;
      if constexpr (std::is_same_v<T, LabelLeaf>)  return x.label;
      if constexpr (std::is_same_v<T, FuncLeaf>)   return x.name;
      return "?leaf";
    }, leaf);
  }

  static std::string leaf_node_to_str(const Tree* t) {
    assert(t && is_leaf(*t));
    return leaf_to_str(*t->leaf);
  }

  static const char* op_to_str(OP op) {
    switch (op) {
      case plus:        return "+=";
      case minus:       return "-=";
      case times:       return "*=";
      case at:          return "&=";
      case left_shift:  return "<<=";
      case right_shift: return ">>=";
    }
    return "?op";
  }

  static const char* cmp_to_str(CMP c) {
    switch (c) {
      case less_than:          return "<";
      case less_than_equal:    return "<=";
      case equal:              return "=";
      case greater_than_equal: return ">=";
      case greater_than:       return ">";
    }
    return "?cmp";
  }

  static std::string compute_prefix_from_program(const Program& p) {
    std::string longest = "L";
    for (auto* f : p.functions) {
      if (!f) continue;
      for (auto* inst : f->instructions) {
        if (auto* lab = dynamic_cast<Instruction_label*>(inst)) {
          std::string s = lab->label_->emit();
          if (!s.empty() && s[0] == ':') s = s.substr(1);
          if (s.size() > longest.size()) longest = s;
        }
      }
    }
    return ":" + longest + "_global_";
  }



  Emitter::Emitter(std::ostream& out) : out_(out) {}

  void Emitter::line(const std::string& s) {
    out_ << "  " << s << "\n";
  }

  std::string Emitter::fresh_tmp() {
    return "%__tmp" + std::to_string(tmp_next_++);
  }



  void GlobalLabel::enter_function(const std::string& fn) { cur_fn = fn; }

  std::string GlobalLabel::make_label(const std::string& l3_label) {
    const std::string key = cur_fn + "|" + l3_label;

    auto it = labelMap.find(key);
    if (it != labelMap.end()) {
      return prefix + std::to_string(it->second);
    }

    int64_t id = next++;
    labelMap.emplace(key, id);
    return prefix + std::to_string(id);
  }

  std::string GlobalLabel::make_fresh_label() {
    return prefix + std::to_string(next++);
  }






  bool AssignTile::match(const Tree& t, Match& m) {
    if (t.kind != TreeType::Assign) return false; 
    const Tree* dst = ptr(t.lhs); 
    const Tree* src = ptr(t.rhs);
    if (!dst || !src) return false; 
    if (!is_leaf(*dst) || !is_leaf(*src)) return false; 

    m.node = &t; 
    m.dst = dst; 
    m.rhs = src; 
    return true; 
  }

  int AssignTile::cost() {
    return 1; 
  } 

  void AssignTile::emit(const Match& m, Emitter& e, GlobalLabel& labeler) {
    e.line(leaf_node_to_str(m.dst) + " <- " + leaf_node_to_str(m.rhs));
  } 


  bool AssignBinOpTile::match(const Tree& t, Match& m) {
    if (t.kind != TreeType::Assign) return false; 
    const Tree* dst = ptr(t.lhs); 
    const Tree* binOp = ptr(t.rhs);
    if (!dst || !binOp) return false; 
    if (!is_leaf(*dst)) return false; 

    if (binOp->kind != TreeType::BinOp) return false; 
    if (!binOp->binOp.has_value()) return false; 
    const Tree* lhs = ptr(binOp->lhs); 
    const Tree* rhs = ptr(binOp->rhs);
    if (!lhs || !rhs) return false; 
    if (!is_leaf(*lhs) || !is_leaf(*rhs)) return false;

    m.node = &t; 
    m.dst = dst; 
    m.lhs = lhs; 
    m.rhs = rhs; 
    m.op = binOp -> binOp; 
    return true; 
  }

  int AssignBinOpTile::cost() {
    return 1; 
  }

  void AssignBinOpTile::emit(const Match& m, Emitter& e, GlobalLabel&) {
    auto dst = leaf_node_to_str(m.dst);
    auto lhs = leaf_node_to_str(m.lhs);
    auto rhs = leaf_node_to_str(m.rhs);
    auto op  = std::string(op_to_str(m.op.value()));

    if (dst == lhs) {
      // x <- x op y  => x op= y
      e.line(dst + " " + op + " " + rhs);
      return;
    }

    if (dst == rhs) {
      // x <- y op x  => need temp
      std::string tmp = e.fresh_tmp();  
      e.line(tmp + " <- " + rhs);
      e.line(dst + " <- " + lhs);
      e.line(dst + " " + op + " " + tmp);
      return;
    }

    // general safe case
    e.line(dst + " <- " + lhs);
    e.line(dst + " " + op + " " + rhs);
  }



  bool AssignCmpTile::match(const Tree& t, Match& m) {
    if (t.kind != TreeType::Assign) return false; 
    const Tree* dst = ptr(t.lhs); 
    const Tree* cmp = ptr(t.rhs);
    if (!dst || !cmp) return false; 
    if (!is_leaf(*dst)) return false; 

    if (cmp->kind != TreeType::Cmp) return false; 
    if (!cmp->cmp.has_value()) return false; 
    const Tree* lhs = ptr(cmp->lhs); 
    const Tree* rhs = ptr(cmp->rhs);
    if (!lhs || !rhs) return false; 
    if (!is_leaf(*lhs) || !is_leaf(*rhs)) return false;

    m.node = &t; 
    m.dst = dst; 
    m.lhs = lhs; 
    m.rhs = rhs; 
    m.cmp = cmp -> cmp; 
    return true; 
  }

  int AssignCmpTile::cost() {
    return 1; 
  }

  void AssignCmpTile::emit(const Match& m, Emitter& e, GlobalLabel& labeler) { 
    if (cmp_to_str(m.cmp.value()) == ">") {
      e.line(leaf_node_to_str(m.dst) + " <- " + leaf_node_to_str(m.rhs) + " " + "<" + " " + leaf_node_to_str(m.lhs));
    } else if (cmp_to_str(m.cmp.value()) == ">=") {
      e.line(leaf_node_to_str(m.dst) + " <- " + leaf_node_to_str(m.rhs) + " " + "<=" + " " + leaf_node_to_str(m.lhs));
    } else {
      e.line(leaf_node_to_str(m.dst) + " <- " + leaf_node_to_str(m.lhs) + " " + cmp_to_str(m.cmp.value()) + " " + leaf_node_to_str(m.rhs));
    }
  }

  bool LoadTile::match(const Tree& t, Match& m) {
    if (t.kind != TreeType::Load) return false; 
    const Tree* dst = ptr(t.lhs); 
    const Tree* src = ptr(t.rhs); 
    if (!dst || !src) return false; 
    if (!is_leaf(*dst) || !is_leaf(*src)) return false; 
      m.node = &t;
      m.dst  = dst;
      m.rhs  = src;
      return true;
  }

  int LoadTile::cost() {
    return 1; 
  } 

  void LoadTile::emit(const Match& m, Emitter& e, GlobalLabel& labeler) {
    e.line(leaf_node_to_str(m.dst) + " <- " + "mem " + leaf_node_to_str(m.rhs) + " 0");
  }


  bool StoreTile::match(const Tree& t, Match& m) {
    if (t.kind != TreeType::Store) return false; 
    const Tree* dst = ptr(t.lhs); 
    const Tree* src = ptr(t.rhs); 
    if (!dst || !src) return false; 
    if (!is_leaf(*dst) || !is_leaf(*src)) return false; 
      m.node = &t;
      m.dst  = dst;
      m.rhs  = src;
      return true;
  }

  int StoreTile::cost() {
    return 1; 
  } 

  void StoreTile::emit(const Match& m, Emitter& e, GlobalLabel& labeler) {
    e.line("mem " + leaf_node_to_str(m.dst) + " 0" + " <- " + leaf_node_to_str(m.rhs)); 
  }

  bool ReturnTile::match(const Tree& t, Match& m) {
    if (t.kind != TreeType::Return) return false; 
    if (t.lhs) {
      if (!is_leaf(*t.lhs)) return false; 
      m.lhs = ptr(t.lhs); 
    }
    m.node = &t; 
    return true; 
  }

  int ReturnTile::cost() {
    return 1;
  }

  void ReturnTile::emit(const Match& m, Emitter &e, GlobalLabel& labeler) {
    if (m.lhs) {
      e.line("rax <- " + leaf_node_to_str(m.lhs));
    }
    e.line("return"); 
  }

  bool BreakTile::match(const Tree& t, Match& m) {
    if (t.kind != TreeType::Break) return false; 
    const Tree* label = ptr(t.lhs);
    if (t.rhs) {
      if (!is_leaf(*t.rhs)) return false; 
      m.rhs = ptr(t.rhs);
    }
    m.node = &t; 
    m.lhs = label; 
    return true; 
  }

  int BreakTile::cost() {
    return 1; 
  }

  void BreakTile::emit(const Match& m, Emitter &e, GlobalLabel& labeler) {
    if (m.rhs) {
      e.line("cjump " + leaf_node_to_str(m.rhs) + " = 1 " + labeler.make_label(leaf_node_to_str(m.lhs)));
    } else {
      e.line("goto " + labeler.make_label(leaf_node_to_str(m.lhs))); 
    }
  }




  TilingEngine::TilingEngine(std::ostream& out, GlobalLabel& labeler)
    : emitter_(out), labeler_(labeler) {
    add_tile(std::make_unique<AssignBinOpTile>());
    add_tile(std::make_unique<AssignCmpTile>());
    add_tile(std::make_unique<AssignTile>());
    add_tile(std::make_unique<LoadTile>());
    add_tile(std::make_unique<StoreTile>());
    add_tile(std::make_unique<ReturnTile>());
    add_tile(std::make_unique<BreakTile>());
  }

  void TilingEngine::add_tile(std::unique_ptr<Tile> t) {
    tiles_.push_back(std::move(t));
  }

  Tile* TilingEngine::select_best_tile(const Tree& t, Match& out_match) const {
    Tile* best = nullptr;
    int best_cost = 1e9;

    for (const auto& tile : tiles_) {
      Match m{};
      if (!tile->match(t, m)) continue;
      int c = tile->cost();
      if (c < best_cost) {
        best = tile.get();
        best_cost = c;
        out_match = m;
      }
    }
    return best;
  }

  void TilingEngine::tile_tree(const Tree& t) {
    Match m{};
    Tile* tile = select_best_tile(t, m);
    if (!tile) {
      std::cerr << "No tile matched tree kind=" << static_cast<int>(t.kind) << "\n";
      assert(false);
    }
    tile->emit(m, emitter_, labeler_);
  }

  void TilingEngine::initialize_function_args(const std::vector<Variable*> var_arguments) {
    std::vector<Variable*> vars = var_arguments;
    emitter_.line(std::to_string(vars.size())); 
    for (size_t idx = 0; idx < vars.size(); idx++) {
      if (idx == 0) emitter_.line(vars[idx]->emit() + " <- rdi");
      if (idx == 1) emitter_.line(vars[idx]->emit() + " <- rsi");
      if (idx == 2) emitter_.line(vars[idx]->emit() + " <- rdx");
      if (idx == 3) emitter_.line(vars[idx]->emit() + " <- rcx");
      if (idx == 4) emitter_.line(vars[idx]->emit() + " <- r8");
      if (idx == 5) emitter_.line(vars[idx]->emit() + " <- r9");
    }
  }

  template <class CallT>
  void TilingEngine::handle_call(const CallT* call) {
    for (size_t idx = 0; idx < call->args_.size(); ++idx) {
      std::string arg = call->args_[idx]->emit();
      if (idx == 0) emitter_.line("rdi <- " + arg);
      if (idx == 1) emitter_.line("rsi <- " + arg);
      if (idx == 2) emitter_.line("rdx <- " + arg);
      if (idx == 3) emitter_.line("rcx <- " + arg);
      if (idx == 4) emitter_.line("r8 <- " + arg);
      if (idx == 5) emitter_.line("r9 <- " + arg);
    }

    CallType c = call->c_;
    if (c == CallType::l3) {

      std::string ret = labeler_.make_fresh_label();
      emitter_.line("mem rsp -8 <- " + ret);
      emitter_.line("call " + call->callee_->emit() + " " +
                    std::to_string(call->args_.size()));
      emitter_.line(ret);
    } else if (c == CallType::print) {
      emitter_.line("call print " + std::to_string(call->args_.size()));
    } else if (c == CallType::input) {
      emitter_.line("call input " + std::to_string(call->args_.size()));
    } else if (c == CallType::allocate) {
      emitter_.line("call allocate " + std::to_string(call->args_.size()));
    } else if (c == CallType::tuple_error) {
      emitter_.line("call tuple-error " + std::to_string(call->args_.size()));
    } else if (c == CallType::tensor_error) {
      emitter_.line("call tensor-error " + std::to_string(call->args_.size()));
    }
  }

  void TilingEngine::codegen (const Node& item) {
    if (auto *t = std::get_if<std::unique_ptr<Tree>>(&item)) {
        tile_tree(*t->get());
      } else if (auto *i = std::get_if<Instruction_label*>(&item)) {
        emitter_.line(labeler_.make_label((*i)->label_->emit())); 
      } else if (auto *i = std::get_if<Instruction_call*>(&item)) {
        handle_call(*i);
      } else if (auto *i = std::get_if<Instruction_call_assignment*>(&item)) {
        handle_call(*i);
        emitter_.line((*i)->dst_->emit() + " <- rax");
    }
  }

  void TilingEngine::tile_function(Function& f) {
    labeler_.enter_function(f.name);
    emitter_.line("(" + f.name);
    initialize_function_args(f.var_arguments);
    for (const auto& ctx : f.contexts) {
      for (auto& nodePtr : ctx.trees) {
        codegen(nodePtr);
      }
    }
    emitter_.line(")");
  }

  void TilingEngine::tile(Program& p) {
    emitter_.line("(@main");
    for (auto* f : p.functions) {
      tile_function(*f);
    }
    emitter_.line(")");
  }

  void tile_program(Program& p, std::ostream& out) {
    GlobalLabel labeler{}; 
    labeler.prefix = compute_prefix_from_program(p);
    TilingEngine eng(out, labeler);
    eng.tile(p);
  }
} 
