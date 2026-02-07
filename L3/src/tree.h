#pragma once 

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <L3.h> 

namespace L3 {
    enum class TreeType { Assign, BinOp, Cmp, Load, Store, Return, Break, Leaf}; 

    struct NumberLeaf { int64_t n; };

    struct VarLeaf { std::string var; }; 

    struct LabelLeaf { std::string label; }; 

    struct FuncLeaf { std::string name; }; 

    using Leaf = std::variant<NumberLeaf, VarLeaf, LabelLeaf, FuncLeaf>;


    struct Tree {
        TreeType kind; 
        std::optional<Leaf> leaf; 
        std::unique_ptr<Tree> lhs; 
        std::unique_ptr<Tree> rhs; 
        std::optional<L3::OP> binOp; 
        std::optional<L3::CMP> cmp; 
    };


    std::unique_ptr<Tree> make_leaf(const Item* item); 
    std::unique_ptr<Tree> make_assign(std::unique_ptr<Tree> lhs, std::unique_ptr<Tree> rhs); 
    std::unique_ptr<Tree> make_binop(OP op, std::unique_ptr<Tree> lhs, std::unique_ptr<Tree> rhs);
    std::unique_ptr<Tree> make_cmp(CMP cmp, std::unique_ptr<Tree> lhs, std::unique_ptr<Tree> rhs); 
    std::unique_ptr<Tree> make_load(std::unique_ptr<Tree> lhs, std::unique_ptr<Tree> rhs); 
    std::unique_ptr<Tree> make_store(std::unique_ptr<Tree> lhs, std::unique_ptr<Tree> rhs); 
    std::unique_ptr<Tree> make_return(std::unique_ptr<Tree> lhs = nullptr);
    std::unique_ptr<Tree> make_break(std::unique_ptr<Tree> lhs, std::unique_ptr<Tree> rhs = nullptr); 
}
