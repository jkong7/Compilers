#include <liveness_analysis.h>

namespace L3 {

  void analyze_liveness(Program& p) {
    LivenessAnalysisBehavior b; 
    b.act(p);
    return;
  }


  void LivenessAnalysisBehavior::act(Program& p) {
    for (auto* f : p.functions) {
      f->accept(*this);
    }
  }

  void LivenessAnalysisBehavior::act(Function& f) {
    cur_func_ = &f;
    cur_i_ = 0;

    f.liveness_data.clear();
    f.liveness_data.resize(f.instructions.size());

    build_label_map();

    cur_i_ = 0;
    for (auto* ins : f.instructions) {
      ins->accept(*this);
      cur_i_++;
    }

    build_successors();
    compute_in_out_fixed_point();

    cur_func_ = nullptr;
  }

  void LivenessAnalysisBehavior::build_label_map() {
    label_to_index_.clear();
    for (size_t i = 0; i < cur_func_->instructions.size(); ++i) {
      if (auto* lab = dynamic_cast<Instruction_label*>(cur_func_->instructions[i])) {
        label_to_index_[lab->label_->emit()] = i;
      }
    }
  }

  void LivenessAnalysisBehavior::build_successors() {
    const size_t n = cur_func_->instructions.size();
    succs_.assign(n, {});

    auto add_fallthrough = [&](size_t i) {
      if (i + 1 < n) succs_[i].push_back(i + 1);
    };

    for (size_t i = 0; i < n; ++i) {
      Instruction* ins = cur_func_->instructions[i];

      if (dynamic_cast<Instruction_return*>(ins) ||
          dynamic_cast<Instruction_return_t*>(ins)) {
        continue;
      }

      if (auto* br = dynamic_cast<Instruction_break_label*>(ins)) {
        auto it = label_to_index_.find(br->label_->emit());
        if (it != label_to_index_.end()) succs_[i].push_back(it->second);
        continue;
      }

      if (auto* brt = dynamic_cast<Instruction_break_t_label*>(ins)) {
        add_fallthrough(i);
        auto it = label_to_index_.find(brt->label_->emit());
        if (it != label_to_index_.end()) succs_[i].push_back(it->second);
        continue;
      }

      add_fallthrough(i);
    }
  }

  void LivenessAnalysisBehavior::compute_in_out_fixed_point() {
    auto& data = cur_func_->liveness_data;
    const size_t n = data.size();

    bool changed = true;
    while (changed) {
      changed = false;

      for (size_t idx = n; idx-- > 0;) {
        auto& ls = data[idx];

        std::unordered_set<std::string> new_out;
        for (size_t s : succs_[idx]) {
          set_union_inplace(new_out, data[s].in);
        }

        auto out_minus_kill = set_minus(new_out, ls.kill);

        std::unordered_set<std::string> new_in = ls.gen;
        set_union_inplace(new_in, out_minus_kill);

        if (new_out != ls.out || new_in != ls.in) {
          ls.out = std::move(new_out);
          ls.in  = std::move(new_in);
          changed = true;
        }
      }
    }
  }

  void LivenessAnalysisBehavior::act(Instruction_assignment& i) {
    auto& ls = cur_func_->liveness_data[cur_i_];
    add_kill_if_var(ls.kill, i.dst_);
    add_use_if_var(ls.gen, i.src_);
  }

  void LivenessAnalysisBehavior::act(Instruction_op& i) {
    auto& ls = cur_func_->liveness_data[cur_i_];
    add_kill_if_var(ls.kill, i.dst_);
    add_use_if_var(ls.gen, i.lhs_);
    add_use_if_var(ls.gen, i.rhs_);
  }

  void LivenessAnalysisBehavior::act(Instruction_cmp& i) {
    auto& ls = cur_func_->liveness_data[cur_i_];
    add_kill_if_var(ls.kill, i.dst_);
    add_use_if_var(ls.gen, i.lhs_);
    add_use_if_var(ls.gen, i.rhs_);
  }

  void LivenessAnalysisBehavior::act(Instruction_load& i) {
    auto& ls = cur_func_->liveness_data[cur_i_];
    add_kill_if_var(ls.kill, i.dst_);
    add_use_if_var(ls.gen, i.src_); 
  }

  void LivenessAnalysisBehavior::act(Instruction_store& i) {
    auto& ls = cur_func_->liveness_data[cur_i_];
    add_use_if_var(ls.gen, i.dst_);
    add_use_if_var(ls.gen, i.src_);   
  }

  void LivenessAnalysisBehavior::act(Instruction_return& i) {
  }

  void LivenessAnalysisBehavior::act(Instruction_return_t& i) {
    auto& ls = cur_func_->liveness_data[cur_i_];
    add_use_if_var(ls.gen, i.ret_);
  }

  void LivenessAnalysisBehavior::act(Instruction_label& i) {
  }

  void LivenessAnalysisBehavior::act(Instruction_break_label& i) {
  }

  void LivenessAnalysisBehavior::act(Instruction_break_t_label& i) {
    auto& ls = cur_func_->liveness_data[cur_i_];
    add_use_if_var(ls.gen, i.t_);
  }

  void LivenessAnalysisBehavior::act(Instruction_call& i) {
    auto& ls = cur_func_->liveness_data[cur_i_];

    if (i.c_ == CallType::l3) {
        add_use_if_var(ls.gen, i.callee_);
    }
    for (Item* a : i.args_) add_use_if_var(ls.gen, a);

  }

  void LivenessAnalysisBehavior::act(Instruction_call_assignment& i) {
    auto& ls = cur_func_->liveness_data[cur_i_];

    add_kill_if_var(ls.kill, i.dst_);
    if (i.c_ == CallType::l3) {
        add_use_if_var(ls.gen, i.callee_);
    }
    for (Item* a : i.args_) add_use_if_var(ls.gen, a);
  }

  void LivenessAnalysisBehavior::set_union_inplace(std::unordered_set<std::string>& dst, const std::unordered_set<std::string>& src) {
    for (const auto& x : src) dst.insert(x);
  }

  std::unordered_set<std::string> LivenessAnalysisBehavior::set_minus(const std::unordered_set<std::string>& a, const std::unordered_set<std::string>& b) {
    std::unordered_set<std::string> out; 
    out.reserve(a.size());
    for (const auto& x : a) {
        if (!b.count(x)) out.insert(x);
    }
    return out; 
  }

  bool LivenessAnalysisBehavior::is_var_item(Item* it) {
    if (!it) return false;
    return it->kind() == ItemType::VariableItem; 
  }

  void LivenessAnalysisBehavior::add_use_if_var(std::unordered_set<std::string>& s, Item* it) {
    if (is_var_item(it)) s.insert(it->emit());
  }

  void LivenessAnalysisBehavior::add_kill_if_var(std::unordered_set<std::string>& s, Item* it) {
    if (is_var_item(it)) s.insert(it->emit());
  }

} 
