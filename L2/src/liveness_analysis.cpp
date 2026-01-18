#include <string>
#include <iostream>
#include <fstream>

#include <liveness_analysis.h>
#include <helper.h> 

using namespace std;

namespace L2{

    bool LivenessAnalysisBehavior::isLivenessContributor(const Item* var) {
        return var->kind() == ItemType::RegisterItem || var->kind() == ItemType::VariableItem || var->kind() == ItemType::MemoryItem;
    }

    void LivenessAnalysisBehavior::print_instruction_gen_kill(size_t cur_i, const livenessSets& ls) {
    std::cout << "gen set: " << cur_i << ":\n";
    for (const auto &s : ls.gen) std::cout << s << "\n";

    std::cout << "kill set: " << cur_i << ":\n";
    for (const auto &s : ls.kill) std::cout << s << "\n";
    }

    void LivenessAnalysisBehavior::act(Program& p) {
        for (Function *f: p.functions) {
            f->accept(*this); 
        }
    }

    void LivenessAnalysisBehavior::act(Function& f) {
        livenessData.emplace_back(f.instructions.size()); 
        for (Instruction *i: f.instructions) {
            i->accept(*this); 
            cur_i++; 
        }
    }

    void LivenessAnalysisBehavior::act(Instruction_assignment& i) {
        auto &ls = livenessData.back()[cur_i]; 
        const Item* dst = i.dst(); 
        const Item* src = i.src(); 
        EmitOptions options; 
        options.livenessAnalysis = true; 
        if (isLivenessContributor(src)) {
            ls.gen.insert(src->emit(options));
        }
        if (isLivenessContributor(dst)) {
            if (dst->kind() == ItemType::MemoryItem) {
                ls.gen.insert(dst->emit(options)); 
            } else {
                ls.kill.insert(dst->emit(options));
            } 
        }
        print_instruction_gen_kill(cur_i, ls); 
    }

    void LivenessAnalysisBehavior::act(Instruction_stack_arg_assignment& i) {
        auto &ls = livenessData.back()[cur_i]; 
        
    }

    void LivenessAnalysisBehavior::act(Instruction_aop& i) {
        
    }

    void LivenessAnalysisBehavior::act(Instruction_sop& i) {
        
    }
    
    void LivenessAnalysisBehavior::act(Instruction_mem_aop& i) {
        
    }

    void LivenessAnalysisBehavior::act(Instruction_cmp_assignment& i) {
        
    }

    void LivenessAnalysisBehavior::act(Instruction_cjump& i) {
        
    }

    void LivenessAnalysisBehavior::act(Instruction_label& i) {
        
    }

    void LivenessAnalysisBehavior::act(Instruction_goto& i) {
        
    }

    void LivenessAnalysisBehavior::act(Instruction_ret& i) {
        
    }

    void LivenessAnalysisBehavior::act(Instruction_call& i) {
        
    }

    void LivenessAnalysisBehavior::act(Instruction_reg_inc_dec& i) {
        
    }

    void LivenessAnalysisBehavior::act(Instruction_lea& i) {
        
    }

    void analyze_liveness(Program p) {
        LivenessAnalysisBehavior b;
        p.accept(b); 
    }
}
