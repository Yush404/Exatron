#pragma once
#include "ast.hpp"
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace exatron {

class TypeError : public std::runtime_error {
public:
    SourceLoc loc;
    TypeError(const std::string& msg, SourceLoc l)
        : std::runtime_error(msg), loc(l) {}
};

struct FnSignature {
    std::vector<TypeInfo> param_types;
    TypeInfo return_type;
};

struct ModelSignature {
    std::string name;
    TypeInfo input_type;
    TypeInfo output_type;
};

class TypeChecker {
public:
    void check_program(Program& program);

private:
    std::unordered_map<std::string, FnSignature> functions_;
    std::unordered_map<std::string, ModelSignature> models_;
    std::vector<std::unordered_map<std::string, TypeInfo>> scopes_;
    TypeInfo current_return_type_{BaseType::Void};

    void push_scope();
    void pop_scope();
    void declare_var(const std::string& name, const TypeInfo& type, SourceLoc loc);
    TypeInfo lookup_var(const std::string& name, SourceLoc loc) const;

    void check_model(ModelDecl& model);
    void check_fn(FnDecl& fn);
    void check_block(BlockStmt& block);
    void check_stmt(Stmt& stmt);
    TypeInfo check_expr(Expr& expr);
    void validate_type(const TypeInfo& type, SourceLoc loc);
    bool block_always_returns(const BlockStmt& block) const;
    bool stmt_always_returns(const Stmt& stmt) const;
};

} // namespace exatron

