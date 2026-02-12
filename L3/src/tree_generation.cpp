#include <tree_generation.h>
#include <tree.h> 

namespace L3 {

    void ContextBehavior::act(Program& p) {
        for (const auto &f : p.functions) {
            f->accept(*this);
        } 
    }

    void ContextBehavior::act(Function& f) {
        cur_function_ = &f;
        contexts.clear();
        contexts.push_back(Context{});
        cur_context = 0;

        for (auto *i : f.instructions) {
            i->accept(*this);
        }
        contexts.erase(
            std::remove_if(
                contexts.begin(), 
                contexts.end(), 
                [](const Context &s) {
                    return s.nodes.empty();
                }
            ),
            contexts.end()
        );

        f.contexts = std::move(contexts); 
        //print_trees(); 
    }

    void ContextBehavior::act(Instruction_assignment& i) {
        std::unique_ptr<Tree> lhs = make_leaf(i.dst_); 
        std::unique_ptr<Tree> rhs = make_leaf(i.src_); 
        std::unique_ptr<Tree> assign_tree = make_assign(std::move(lhs), std::move(rhs)); 
        contexts[cur_context].nodes.push_back(std::move(assign_tree)); 
    }

    void ContextBehavior::act(Instruction_op& i) {
        std::unique_ptr<Tree> lhs = make_leaf(i.lhs_); 
        std::unique_ptr<Tree> rhs = make_leaf(i.rhs_);
        std::unique_ptr<Tree> dst = make_leaf(i.dst_); 
        std::unique_ptr<Tree> binop_tree = make_binop(i.op_, std::move(lhs), std::move(rhs));
        std::unique_ptr<Tree> assign_tree = make_assign(std::move(dst), std::move(binop_tree));
        contexts[cur_context].nodes.push_back(std::move(assign_tree)); 
    }

    void ContextBehavior::act(Instruction_cmp& i) {
        std::unique_ptr<Tree> lhs = make_leaf(i.lhs_); 
        std::unique_ptr<Tree> rhs = make_leaf(i.rhs_);
        std::unique_ptr<Tree> dst = make_leaf(i.dst_); 
        std::unique_ptr<Tree> cmp_tree = make_cmp(i.cmp_, std::move(lhs), std::move(rhs));
        std::unique_ptr<Tree> assign_tree = make_assign(std::move(dst), std::move(cmp_tree));
        contexts[cur_context].nodes.push_back(std::move(assign_tree));     
    }

    void ContextBehavior::act(Instruction_load& i) {
        std::unique_ptr<Tree> lhs = make_leaf(i.dst_); 
        std::unique_ptr<Tree> rhs = make_leaf(i.src_);
        std::unique_ptr<Tree> load_tree = make_load(std::move(lhs), std::move(rhs));
        contexts[cur_context].nodes.push_back(std::move(load_tree));
    }

    void ContextBehavior::act(Instruction_store& i) {
        std::unique_ptr<Tree> lhs = make_leaf(i.dst_); 
        std::unique_ptr<Tree> rhs = make_leaf(i.src_);
        std::unique_ptr<Tree> store_tree = make_store(std::move(lhs), std::move(rhs));
        contexts[cur_context].nodes.push_back(std::move(store_tree));    
    }

    void ContextBehavior::act(Instruction_return& i) {
        std::unique_ptr<Tree> return_tree = make_return(); 
        contexts[cur_context].nodes.push_back(std::move(return_tree));
        end_context();
    }

    void ContextBehavior::act(Instruction_return_t& i) {
        std::unique_ptr<Tree> t = make_leaf(i.ret_);
        std::unique_ptr<Tree> return_tree = make_return(std::move(t)); 
        contexts[cur_context].nodes.push_back(std::move(return_tree));
        end_context();
    }

    void ContextBehavior::act(Instruction_label& i) {
        contexts[cur_context].nodes.push_back(&i);
        end_context();
    }

    void ContextBehavior::act(Instruction_break_label& i) {
        std::unique_ptr<Tree> label = make_leaf(i.label_);
        std::unique_ptr<Tree> break_tree = make_break(std::move(label));
        contexts[cur_context].nodes.push_back(std::move(break_tree));
        end_context();
    }

    void ContextBehavior::act(Instruction_break_t_label& i) {
        std::unique_ptr<Tree> label = make_leaf(i.label_);
        std::unique_ptr<Tree> t = make_leaf(i.t_);
        std::unique_ptr<Tree> break_tree = make_break(std::move(label), std::move(t));
        contexts[cur_context].nodes.push_back(std::move(break_tree));
        end_context();
    }

    void ContextBehavior::act(Instruction_call& i) {
        contexts[cur_context].nodes.push_back(&i);
        end_context();
    }

    void ContextBehavior::act(Instruction_call_assignment& i) {
        contexts[cur_context].nodes.push_back(&i);
        end_context(); 
    }

    void ContextBehavior::end_context() {
        contexts.push_back(Context{});
        cur_context++; 
    }

    void make_trees(Program &p) {
        ContextBehavior cb; 
        cb.act(p);
    }





    static const char* op_to_str(OP op) {
    switch (op) {
        case plus:        return "+";
        case minus:       return "-";
        case times:       return "*";
        case at:          return "@";
        case left_shift:  return "<<";
        case right_shift: return ">>";
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

    static void print_tree_rec(const Tree* t, int indent, std::ostream& out) {
    if (!t) {
        out << std::string(indent, ' ') << "(null)\n";
        return;
    }

    auto ind = std::string(indent, ' ');

    switch (t->kind) {
        case TreeType::Leaf: {
        out << ind << "Leaf ";
        if (t->leaf.has_value()) out << leaf_to_str(*t->leaf);
        else out << "(empty)";
        out << "\n";
        return;
        }

        case TreeType::Assign: {
        out << ind << "Assign\n";
        print_tree_rec(t->lhs.get(), indent + 2, out);
        print_tree_rec(t->rhs.get(), indent + 2, out);
        return;
        }

        case TreeType::BinOp: {
        out << ind << "BinOp ";
        if (t->binOp.has_value()) out << op_to_str(*t->binOp);
        else out << "(missing-op)";
        out << "\n";
        print_tree_rec(t->lhs.get(), indent + 2, out);
        print_tree_rec(t->rhs.get(), indent + 2, out);
        return;
        }

        case TreeType::Cmp: {
        out << ind << "Cmp ";
        if (t->cmp.has_value()) out << cmp_to_str(*t->cmp);
        else out << "(missing-cmp)";
        out << "\n";
        print_tree_rec(t->lhs.get(), indent + 2, out);
        print_tree_rec(t->rhs.get(), indent + 2, out);
        return;
        }

        case TreeType::Load: {
        out << ind << "Load\n";
        print_tree_rec(t->lhs.get(), indent + 2, out);
        print_tree_rec(t->rhs.get(), indent + 2, out);
        return;
        }

        case TreeType::Store: {
        out << ind << "Store\n";
        print_tree_rec(t->lhs.get(), indent + 2, out);
        print_tree_rec(t->rhs.get(), indent + 2, out);
        return;
        }

        case TreeType::Return: {
        out << ind << "Return\n";
        if (t->lhs) print_tree_rec(t->lhs.get(), indent + 2, out);
        return;
        }

        case TreeType::Break: {
        out << ind << "Break\n";
        print_tree_rec(t->lhs.get(), indent + 2, out);
        if (t->rhs) print_tree_rec(t->rhs.get(), indent + 2, out);
        return;
        }
    }

    out << ind << "(unknown TreeType)\n";
    }

    void ContextBehavior::print_trees() {
    if (!cur_function_) {
        std::cerr << "[print_trees] No current function set.\n";
        return;
    }

    std::cout << "=== Trees for function: " << cur_function_->name << " ===\n";
    const auto& ctxs = cur_function_->contexts;

    for (size_t ci = 0; ci < ctxs.size(); ++ci) {
        std::cout << "\n[Context " << ci << "] trees=" << ctxs[ci].nodes.size() << "\n";
        for (size_t ti = 0; ti < ctxs[ci].nodes.size(); ++ti) {
        std::cout << "  (Tree " << ti << ")\n";
        if (auto* t = std::get_if<std::unique_ptr<Tree>>(&ctxs[ci].nodes[ti])) {
            if (t->get()) {
            print_tree_rec(t->get(), 4, std::cout);
            }
        } else if (auto *i = std::get_if<Instruction_label*>(&ctxs[ci].nodes[ti])) {
            std::cout << "  " << "LABEL instruction\n";
        } else if (auto *i = std::get_if<Instruction_call*>(&ctxs[ci].nodes[ti])) {
            std::cout << "  " << "CALL instruction\n";
        } else if (auto *i = std::get_if<Instruction_call_assignment*>(&ctxs[ci].nodes[ti])) {
            std::cout << "  " << "CALL assignment instruction\n";
        }
        }
    }
    std::cout << "\n=== end ===\n";
    }
} 
