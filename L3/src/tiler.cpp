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
      if constexpr (std::is_same_v<T, LabelLeaf>)  return ":" + x.label;
      if constexpr (std::is_same_v<T, FuncLeaf>)   return "@" + x.name;
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
      case at:          return "@=";
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




  Emitter::Emitter(std::ostream& out) : out_(out) {}

  void Emitter::line(const std::string& s) {
    out_ << "  " << s << "\n";
    ++num_instructions_;
  }

  int64_t Emitter::num_instructions() const {
    return num_instructions_;
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

  void AssignTile::emit(const Match& m, Emitter& e) {
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

  void AssignBinOpTile::emit(const Match& m, Emitter& e) { // a <- b + c
    e.line(leaf_node_to_str(m.dst) + " <- " + leaf_node_to_str(m.lhs)); 
    e.line(leaf_node_to_str(m.dst) + " " + op_to_str(m.op.value()) + " " + leaf_node_to_str(m.rhs));
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
    m.cmp = rhs -> cmp; 
    return true; 
  }

  int AssignCmpTile::cost() {
    return 1; 
  }

  void AssignCmpTile::emit(const Match& m, Emitter& e) { 
    if (cmp_to_str(m.cmp.value()) == ">") {
      e.line(leaf_node_to_str(m.dst) + " <- " + leaf_node_to_str(m.rhs) + " " + "<" + " " + leaf_node_to_str(m.lhs));
    } else if (cmp_to_str(m.cmp.value()) == ">=") {
      e.line(leaf_node_to_str(m.dst) + " <- " + leaf_node_to_str(m.rhs) + " " + "<=" + " " + leaf_node_to_str(m.lhs));
    } else {
      e.line(leaf_node_to_str(m.dst) + " <- " + leaf_node_to_str(m.rhs) + " " + cmp_to_str(m.cmp.value()) + " " + leaf_node_to_str(m.lhs));
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

  void LoadTile::emit(const Match& m, Emitter& e) {
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

  void StoreTile::emit(const Match& m, Emitter& e) {
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

  void ReturnTile::emit(const Match& m, Emitter &e) {
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

  void BreakTile::emit(const Match& m, Emitter &e) {
    e.line("goto " + leaf_node_to_str(m.lhs)); 
  }




  TilingEngine::TilingEngine(std::ostream& out)
    : emitter_(out) {
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
    tile->emit(m, emitter_);
  }

  void TilingEngine::tile_function(Function& f) {
    for (const auto& ctx : f.contexts) {
      for (const auto& treePtr : ctx.trees) {
        if (treePtr) tile_tree(*treePtr);
      }
    }
  }

  void TilingEngine::tile(Program& p) {
    for (auto* f : p.functions) {
      if (!f) continue;
      tile_function(*f);
    }
  }

  void tile_program(Program& p, std::ostream& out) {
    TilingEngine eng(out);
    eng.tile(p);
  }
} 
