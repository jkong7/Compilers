#include <context_generation.h>

namespace L3 {

    void ContextBehavior::act(Program& p) {
        for (const auto &f : p.functions) {
            f->accept(*this);
        } 
    }

    void ContextBehavior::act(Function& f) {
        contexts.clear();
        contexts.push_back(Context{});
        cur_context = 0;

        for (auto *i : f.instructions) {
            cur_instruction = i;
            i->accept(*this);
        }
        contexts.erase(
            std::remove_if(
                contexts.begin(), 
                contexts.end(), 
                [](const Context &s) {
                    return s.instructions.empty();
                }
            ),
            contexts.end()
        );

        f.contexts = contexts; 
    }

    void ContextBehavior::act(Instruction_assignment& i) {
        contexts[cur_context].instructions.push_back(cur_instruction);
    }

    void ContextBehavior::act(Instruction_op& i) {
        contexts[cur_context].instructions.push_back(cur_instruction);
    }

    void ContextBehavior::act(Instruction_cmp& i) {
        contexts[cur_context].instructions.push_back(cur_instruction);
    }

    void ContextBehavior::act(Instruction_load& i) {
        contexts[cur_context].instructions.push_back(cur_instruction);
    }

    void ContextBehavior::act(Instruction_store& i) {
        contexts[cur_context].instructions.push_back(cur_instruction);
    }

    void ContextBehavior::act(Instruction_return& i) {
        contexts[cur_context].instructions.push_back(cur_instruction);
        end_context();
    }

    void ContextBehavior::act(Instruction_return_t& i) {
        contexts[cur_context].instructions.push_back(cur_instruction);
        end_context();
    }

    void ContextBehavior::act(Instruction_label& i) {
        end_context();
    }

    void ContextBehavior::act(Instruction_break_label& i) {
        contexts[cur_context].instructions.push_back(cur_instruction);
        end_context();
    }

    void ContextBehavior::act(Instruction_break_t_label& i) {
        contexts[cur_context].instructions.push_back(cur_instruction);
        end_context();
    }

    void ContextBehavior::act(Instruction_call& i) {
        end_context();
    }

    void ContextBehavior::act(Instruction_call_assignment& i) {
        end_context(); 
    }

    void ContextBehavior::end_context() {
        contexts.push_back(Context{});
        cur_context++; 
    }

} 
