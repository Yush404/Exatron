#pragma once
#include "ast.hpp"
#include "token.hpp"
#include <vector>
#include <stdexcept>

namespace exatron {

class ParseError : public std::runtime_error {
public:
    SourceLoc loc;
    ParseError(const std::string& msg, SourceLoc l)
        : std::runtime_error(msg), loc(l) {}
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    Program parse_program();

private:
    std::vector<Token> tokens_;
    std::size_t current_ = 0;

    // Navigation helpers
    const Token& peek(std::size_t offset = 0) const;
    const Token& previous() const;
    bool is_at_end() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    const Token& consume(TokenType type, const std::string& error_msg);

    // Top-level declarations
    ModelDecl parse_model_decl();
    LayerDecl parse_layer_decl();
    ActivationKind parse_activation();
    FnDecl parse_fn_decl();
    TypeInfo parse_type();

    // Statements
    std::unique_ptr<BlockStmt> parse_block();
    StmtPtr parse_stmt();
    StmtPtr parse_let_stmt();
    StmtPtr parse_assign_or_expr_stmt();
    StmtPtr parse_if_stmt();
    StmtPtr parse_while_stmt();
    StmtPtr parse_for_stmt();
    StmtPtr parse_return_stmt();
    StmtPtr parse_print_stmt();

    // Expressions (precedence climbing / recursive descent)
    ExprPtr parse_expr();
    ExprPtr parse_logic_or();
    ExprPtr parse_logic_and();
    ExprPtr parse_equality();
    ExprPtr parse_comparison();
    ExprPtr parse_term();
    ExprPtr parse_factor();
    ExprPtr parse_unary();
    ExprPtr parse_postfix();
    ExprPtr parse_primary();
    ExprPtr parse_tensor_lit();
};

} // namespace exatron

