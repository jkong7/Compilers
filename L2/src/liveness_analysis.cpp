#include <string>
#include <iostream>
#include <fstream>

#include <liveness_analysis.h>
#include <helper.h> 

using namespace std;

namespace L2{
    void LivenessAnalysisBehavior::act(Program& p) {
        for (Function *f: p.functions) {
            f->accept(*this); 
        }
    }

    void LivenessAnalysisBehavior::act(Function& f) {
        livenessData.emplace_back(f.instructions.size()); 
        for (Instruction *i: f.instructions) {
            i->accept(*this); 
        }
    }

    void LivenessAnalysisBehavior::act(Instruction_assignment& i) {

    }

    void LivenessAnalysisBehavior::act(Instruction_stack_arg_assignment& i) {
        
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
}
