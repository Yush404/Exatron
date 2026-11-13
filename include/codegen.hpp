#pragma once
#include "ast.hpp"
#include <sstream>
#include <string>

namespace exatron {

class CodeGenerator {
public:
    std::string generate(const Program& program);

private:
    std::ostringstream out_;
    int indent_level_ = 0;
    int loop_counter_ = 0;

    std::string mangle(const std::string& name) const;
    void emit_indent();
    void emit_model(const ModelDecl& model);
    void emit_fn_decl(const FnDecl& fn);
    void emit_block(const BlockStmt& block, bool include_braces = true);
    void emit_stmt(const Stmt& stmt);
    void emit_expr(const Expr& expr);
    std::string map_type(const TypeInfo& type) const;
};

} // namespace exatron

