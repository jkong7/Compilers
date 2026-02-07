#include <tree.h> 

namespace L3 {
    std::unique_ptr<Tree> make_leaf(const Item* item) {
        auto t = std::make_unique<Tree>();
        t->kind = TreeType::Leaf; 

        switch (item->kind()) {
            case ItemType::NumberItem: {
                const auto& n = static_cast<const Number*>(item);
                t->leaf = NumberLeaf{ n->number_ };
                break; 
            }
            case ItemType::VariableItem: {
                const auto& v = static_cast<const Variable*>(item); 
                t->leaf = VarLeaf{ v->var_ };
                break;
            }
            case ItemType::LabelItem: {
                const auto& l = static_cast<const Label*>(item); 
                t->leaf = LabelLeaf{ l->label_ };
                break; 
            }
            case ItemType::FuncItem: {
                const auto& f = static_cast<const Func*>(item); 
                t->leaf = FuncLeaf{ f->function_label_ };
                break;
            }
        }
        return t; 
    }

    std::unique_ptr<Tree> make_assign(std::unique_ptr<Tree> lhs, std::unique_ptr<Tree> rhs) {
        auto t = std::make_unique<Tree>(); 
        t->kind = TreeType::Assign; 
        t->lhs = std::move(lhs);
        t->rhs = std::move(rhs); 
        return t; 
    }

    std::unique_ptr<Tree> make_binop(OP op, std::unique_ptr<Tree> lhs, std::unique_ptr<Tree> rhs) {
        auto t = std::make_unique<Tree>(); 
        t->kind = TreeType::BinOp; 
        t->binOp = op;
        t->lhs = std::move(lhs); 
        t->rhs = std::move(rhs);
        return t; 
    }

    std::unique_ptr<Tree> make_cmp(CMP cmp, std::unique_ptr<Tree> lhs, std::unique_ptr<Tree> rhs) {
        auto t = std::make_unique<Tree>(); 
        t->kind = TreeType::Cmp; 
        t->cmp = cmp;
        t->lhs = std::move(lhs); 
        t->rhs = std::move(rhs);
        return t; 
    }

    std::unique_ptr<Tree> make_load(std::unique_ptr<Tree> lhs, std::unique_ptr<Tree> rhs) {
        auto t = std::make_unique<Tree>();
        t->kind = TreeType::Load;
        t->lhs = std::move(lhs); 
        t->rhs = std::move(rhs); 
        return t; 
    }

    std::unique_ptr<Tree> make_store(std::unique_ptr<Tree> lhs, std::unique_ptr<Tree> rhs) {
        auto t = std::make_unique<Tree>(); 
        t->kind = TreeType::Store; 
        t->lhs = std::move(lhs); 
        t->rhs = std::move(rhs); 
        return t; 
    }

    std::unique_ptr<Tree> make_return(std::unique_ptr<Tree> lhs) {
        auto t = std::make_unique<Tree>(); 
        t->kind = TreeType::Return; 
        if (lhs) {
            t->lhs = std::move(lhs);
        }
        return t; 
    }

    std::unique_ptr<Tree> make_break(std::unique_ptr<Tree> lhs, std::unique_ptr<Tree> rhs) {
        auto t = std::make_unique<Tree>(); 
        t->kind = TreeType::Break; 
        t->lhs = std::move(lhs); 
        if (rhs) {
            t->rhs = std::move(rhs); 
        }
        return t; 
    }
}