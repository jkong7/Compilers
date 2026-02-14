#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <algorithm>

#include <L3.h>

namespace L3 {


  class LivenessAnalysisBehavior : public Behavior {
  public:
    LivenessAnalysisBehavior() = default;

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
    void act(Instruction_break_label& i) override;
    void act(Instruction_break_t_label& i) override;
    void act(Instruction_call& i) override;
    void act(Instruction_call_assignment& i) override;

  private:
    Function* cur_func_ = nullptr;
    size_t cur_i_ = 0;
    std::unordered_map<std::string, size_t> label_to_index_;
    std::vector<std::vector<size_t>> succs_;

    static bool is_var_item(Item* it);
    static void add_use_if_var(std::unordered_set<std::string>& s, Item* it);
    static void add_kill_if_var(std::unordered_set<std::string>& s, Item* it);

    static void set_union_inplace(std::unordered_set<std::string>& dst, const std::unordered_set<std::string>& src);
    static std::unordered_set<std::string> set_minus(const std::unordered_set<std::string>& a, const std::unordered_set<std::string>& b);

    void build_label_map();
    void build_successors();
    void compute_in_out_fixed_point();
  };

  void analyze_liveness(Program& p);

} 
