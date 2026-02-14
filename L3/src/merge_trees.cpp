#include <merge_trees.h>

namespace L3 {


static bool leaf_is_var(const Leaf &leaf, std::string &out_var) {
  if (const auto *v = std::get_if<VarLeaf>(&leaf)) {  
    out_var = v->var;
    return true;
  }
  return false;
}

static bool tree_defines_var(const Tree *t, std::string &out_var) {
  if (!t) return false;
  if (t->kind != TreeType::Assign) return false;
  if (!t->lhs) return false;
  if (t->lhs->kind != TreeType::Leaf || !t->lhs->leaf.has_value()) return false;
  return leaf_is_var(*t->lhs->leaf, out_var);
}


static bool tree_uses_var(const Tree *t, const std::string &var) {
  if (!t) return false;

  if (t->kind == TreeType::Leaf && t->leaf.has_value()) {
    if (auto *v = std::get_if<VarLeaf>(&*t->leaf)) {
      if (v->var == var) return true;
    }
  }

  if (t->lhs && tree_uses_var(t->lhs.get(), var)) return true;
  if (t->rhs && tree_uses_var(t->rhs.get(), var)) return true;

  return false;
}


static std::unique_ptr<Tree> clone_tree(const Tree *t) {
  if (!t) return nullptr;
  auto nt = std::make_unique<Tree>();

  nt->kind = t->kind;
  nt->leaf = t->leaf;
  nt->binOp = t->binOp;
  nt->cmp = t->cmp;

  if (t->lhs) nt->lhs = clone_tree(t->lhs.get());
  if (t->rhs) nt->rhs = clone_tree(t->rhs.get());
  return nt;
}


static void substitute_var_in_subtree(std::unique_ptr<Tree> &node,
                                      const std::string &var_name,
                                      const Tree *replacement) {
  if (!node) return;

  Tree *t = node.get();

  if (t->kind == TreeType::Leaf && t->leaf.has_value()) {
    if (auto *v = std::get_if<VarLeaf>(&*t->leaf)) {
      if (v->var == var_name) {
        node = clone_tree(replacement);
        return;
      }
    }
  }

  if (t->lhs) substitute_var_in_subtree(t->lhs, var_name, replacement);
  if (t->rhs) substitute_var_in_subtree(t->rhs, var_name, replacement);
}


static void substitute_uses_of_var(std::unique_ptr<Tree> &root,
                                   const std::string &var_name,
                                   const Tree *replacement) {
  if (!root || !replacement) return;
  Tree *t = root.get();

  if (t->kind == TreeType::Assign && t->rhs) {
    substitute_var_in_subtree(t->rhs, var_name, replacement);
  } else {
    substitute_var_in_subtree(root, var_name, replacement);
  }
}


static bool find_def_use_var(const livenessSets &L2,
                             const livenessSets &L1,
                             std::string &out_var) {
  for (const auto &v : L2.kill) {
    if (L1.gen.count(v)) {
      out_var = v;
      return true;
    }
  }
  return false;
}

static livenessSets merge_liveness(const livenessSets &L2,
                                   const livenessSets &L1) {
  livenessSets L21;

  // kill21 = kill1 ∪ kill2
  L21.kill = L2.kill;
  for (const auto &v : L1.kill) {
    L21.kill.insert(v);
  }

  // gen21 = gen1 ∪ (gen2 − kill1)
  L21.gen = L1.gen;
  for (const auto &v : L2.gen) {
    if (!L1.kill.count(v)) {
      L21.gen.insert(v);
    }
  }

  // out21 = out1
  L21.out = L1.out;

  // in21 = gen21 ∪ (out21 − kill21)
  std::unordered_set<std::string> out_minus_kill;
  out_minus_kill.reserve(L21.out.size());
  for (const auto &v : L21.out) {
    if (!L21.kill.count(v)) {
      out_minus_kill.insert(v);
    }
  }

  L21.in = L21.gen;
  for (const auto &v : out_minus_kill) {
    L21.in.insert(v);
  }

  return L21;
}

static bool try_merge_pair(std::vector<Node> &nodes,
                           std::vector<livenessSets> &lives,
                           size_t t2_idx,
                           size_t t1_idx) {
  // Both need to be trees
  auto *t2_uptr = std::get_if<std::unique_ptr<Tree>>(&nodes[t2_idx]);
  auto *t1_uptr = std::get_if<std::unique_ptr<Tree>>(&nodes[t1_idx]);
  if (!t2_uptr || !t1_uptr) return false;
  if (!t2_uptr->get() || !t1_uptr->get()) return false;

  Tree *t2 = t2_uptr->get();
  Tree *t1 = t1_uptr->get();

  auto &L2 = lives[t2_idx];
  auto &L1 = lives[t1_idx];

  // 1) T1 must use a variable v defined by T2 
  std::string v;
  if (!find_def_use_var(L2, L1, v)) {
    return false;
  }

  // 2) v must be dead after T1
  if (L1.out.count(v)) {
    return false;
  }

  // 3) out(T2) == in(T1)   (B: no interfering uses/defs between them)
  if (L2.out != L1.in) {
    return false;
  }

  {
    std::string def_var;
    if (!tree_defines_var(t2, def_var) || def_var != v) {
      return false;
    }
  }

  if (!tree_uses_var(t1, v)) {
    return false;
  }

  if (t2->kind != TreeType::Assign || !t2->rhs) {
    return false;
  }
  const Tree *rhs_of_T2 = t2->rhs.get();

  substitute_uses_of_var(*t1_uptr, v, rhs_of_T2);

  livenessSets merged = merge_liveness(L2, L1);
  lives[t1_idx] = std::move(merged);

  nodes.erase(nodes.begin() + static_cast<std::ptrdiff_t>(t2_idx));
  lives.erase(lives.begin() + static_cast<std::ptrdiff_t>(t2_idx));

  return true;
}


static std::vector<std::vector<livenessSets>>
build_context_liveness(Function &f) {
  std::vector<std::vector<livenessSets>> ctx_live;
  ctx_live.resize(f.contexts.size());

  size_t ins_idx = 0;
  for (size_t ci = 0; ci < f.contexts.size(); ++ci) {
    auto &nodes = f.contexts[ci].nodes;
    ctx_live[ci].resize(nodes.size());
    for (size_t ni = 0; ni < nodes.size(); ++ni) {
      assert(ins_idx < f.liveness_data.size());
      ctx_live[ci][ni] = f.liveness_data[ins_idx++];
    }
  }

  assert(ins_idx == f.liveness_data.size());
  return ctx_live;
}

static void merge_trees_in_function(Function &f) {
  if (f.contexts.empty()) return;

  auto ctx_live = build_context_liveness(f);

  for (size_t ci = 0; ci < f.contexts.size(); ++ci) {
    auto &nodes = f.contexts[ci].nodes;
    auto &lives = ctx_live[ci];

    bool changed = true;
    while (changed) {
      changed = false;

      // T2 = nodes[j-1], T1 = nodes[j]
      for (size_t j = 1; j < nodes.size(); ++j) {
        size_t t1_idx = j;
        size_t t2_idx = j - 1;

        if (try_merge_pair(nodes, lives, t2_idx, t1_idx)) {
          changed = true;
          if (j > 0) --j;
        }
      }
    }
  }

}

void merge_trees(Program &p) {
  for (auto *f : p.functions) {
    merge_trees_in_function(*f);
  }
}

} 
