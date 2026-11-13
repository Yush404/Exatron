#include "parser.hpp"
#include <stdexcept>

namespace exatron {

// ── Helpers ─────────────────────────────────────────────────────────────────

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

const Token& Parser::peek(std::size_t offset) const {
    std::size_t idx = current_ + offset;
    if (idx >= tokens_.size()) return tokens_.back(); // Eof
    return tokens_[idx];
}

const Token& Parser::previous() const {
    return tokens_[current_ - 1];
}

bool Parser::is_at_end() const {
    return peek().type == TokenType::Eof;
}

const Token& Parser::advance() {
    if (!is_at_end()) ++current_;
    return previous();
}

bool Parser::check(TokenType type) const {
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

const Token& Parser::consume(TokenType type, const std::string& error_msg) {
    if (check(type)) return advance();
    auto& tok = peek();
    throw ParseError("[Parse Error] Line " + std::to_string(tok.loc.line)
        + ", Col " + std::to_string(tok.loc.col) + ": " + error_msg
        + " (found '" + tok.lexeme + "')", tok.loc);
}

// ── Top-Level ───────────────────────────────────────────────────────────────

Program Parser::parse_program() {
    Program program;
    while (!is_at_end()) {
        if (match(TokenType::Model)) {
            program.models.push_back(parse_model_decl());
        } else if (match(TokenType::Fn)) {
            program.functions.push_back(parse_fn_decl());
        } else {
            auto& tok = peek();
            throw ParseError("[Parse Error] Line " + std::to_string(tok.loc.line)
                + ", Col " + std::to_string(tok.loc.col)
                + ": Expected 'model' or 'fn' declaration at top level (found '"
                + tok.lexeme + "')", tok.loc);
        }
    }
    return program;
}

// ── Model ───────────────────────────────────────────────────────────────────

ModelDecl Parser::parse_model_decl() {
    ModelDecl model;
    model.loc = previous().loc;

    auto& name_tok = consume(TokenType::Ident, "Expected model name");
    model.name = name_tok.lexeme;

    consume(TokenType::LBrace, "Expected '{' after model name");

    // input: tensor<...>;
    consume(TokenType::Input, "Expected 'input' declaration in model");
    consume(TokenType::Colon, "Expected ':' after 'input'");
    model.input_type = parse_type();
    consume(TokenType::Semicolon, "Expected ';' after input type");

    // layer declarations
    while (match(TokenType::Layer)) {
        model.layers.push_back(parse_layer_decl());
    }
    if (model.layers.empty()) {
        throw ParseError("[Parse Error] Line " + std::to_string(peek().loc.line)
            + ", Col " + std::to_string(peek().loc.col)
            + ": Model must contain at least one layer", peek().loc);
    }

    consume(TokenType::RBrace, "Expected '}' after model body");
    return model;
}

LayerDecl Parser::parse_layer_decl() {
    LayerDecl layer;
    layer.loc = previous().loc;

    auto& name_tok = consume(TokenType::Ident, "Expected layer name");
    layer.name = name_tok.lexeme;

    consume(TokenType::Colon, "Expected ':' after layer name");
    consume(TokenType::Dense, "Expected 'Dense' layer type");
    consume(TokenType::LParen, "Expected '(' after 'Dense'");

    auto& in_tok = consume(TokenType::IntLit, "Expected integer for in_features");
    layer.in_features = std::stoi(in_tok.lexeme);
    consume(TokenType::Comma, "Expected ',' after in_features");

    auto& out_tok = consume(TokenType::IntLit, "Expected integer for out_features");
    layer.out_features = std::stoi(out_tok.lexeme);
    consume(TokenType::Comma, "Expected ',' after out_features");

    layer.activation = parse_activation();

    consume(TokenType::RParen, "Expected ')' after activation");
    consume(TokenType::Semicolon, "Expected ';' after layer declaration");
    return layer;
}

ActivationKind Parser::parse_activation() {
    if (match(TokenType::Relu))    return ActivationKind::Relu;
    if (match(TokenType::Sigmoid)) return ActivationKind::Sigmoid;
    if (match(TokenType::Tanh))    return ActivationKind::Tanh;
    if (match(TokenType::Softmax)) return ActivationKind::Softmax;
    if (match(TokenType::Linear))  return ActivationKind::Linear;
    auto& tok = peek();
    throw ParseError("[Parse Error] Line " + std::to_string(tok.loc.line)
        + ", Col " + std::to_string(tok.loc.col)
        + ": Expected activation function (relu, sigmoid, tanh, softmax, linear) (found '"
        + tok.lexeme + "')", tok.loc);
}

// ── Functions ───────────────────────────────────────────────────────────────

FnDecl Parser::parse_fn_decl() {
    FnDecl fn;
    fn.loc = previous().loc;

    auto& name_tok = consume(TokenType::Ident, "Expected function name");
    fn.name = name_tok.lexeme;

    consume(TokenType::LParen, "Expected '(' after function name");

    // Parse parameter list
    if (!check(TokenType::RParen)) {
        do {
            Param p;
            auto& pname = consume(TokenType::Ident, "Expected parameter name");
            p.name = pname.lexeme;
            p.loc = pname.loc;
            consume(TokenType::Colon, "Expected ':' after parameter name");
            p.type = parse_type();
            fn.params.push_back(std::move(p));
        } while (match(TokenType::Comma));
    }

    consume(TokenType::RParen, "Expected ')' after parameters");
    consume(TokenType::Arrow, "Expected '->' before return type");
    fn.return_type = parse_type();
    fn.body = parse_block();
    return fn;
}

// ── Type Parsing ────────────────────────────────────────────────────────────

TypeInfo Parser::parse_type() {
    if (match(TokenType::IntType))    return TypeInfo{BaseType::Int};
    if (match(TokenType::FloatType))  return TypeInfo{BaseType::Float};
    if (match(TokenType::BoolType))   return TypeInfo{BaseType::Bool};
    if (match(TokenType::StringType)) return TypeInfo{BaseType::String};
    if (match(TokenType::VoidType))   return TypeInfo{BaseType::Void};

    if (match(TokenType::TensorType)) {
        consume(TokenType::Lt, "Expected '<' after 'tensor'");
        std::vector<int> dims;
        auto& dim_tok = consume(TokenType::IntLit, "Expected integer dimension");
        int d = std::stoi(dim_tok.lexeme);
        if (d <= 0) {
            throw ParseError("[Parse Error] Line " + std::to_string(dim_tok.loc.line)
                + ", Col " + std::to_string(dim_tok.loc.col)
                + ": Tensor dimension must be positive", dim_tok.loc);
        }
        dims.push_back(d);
        while (match(TokenType::Comma)) {
            auto& dtok = consume(TokenType::IntLit, "Expected integer dimension");
            int dd = std::stoi(dtok.lexeme);
            if (dd <= 0) {
                throw ParseError("[Parse Error] Line " + std::to_string(dtok.loc.line)
                    + ", Col " + std::to_string(dtok.loc.col)
                    + ": Tensor dimension must be positive", dtok.loc);
            }
            dims.push_back(dd);
            if (dims.size() > 2) {
                throw ParseError("[Parse Error] Line " + std::to_string(dtok.loc.line)
                    + ", Col " + std::to_string(dtok.loc.col)
                    + ": Tensors support at most 2 dimensions", dtok.loc);
            }
        }
        consume(TokenType::Gt, "Expected '>' after tensor dimensions");

        // Normalize 1D tensor<N> to 2D {1, N} row vector
        if (dims.size() == 1) {
            dims.insert(dims.begin(), 1);
        }
        return TypeInfo{BaseType::Tensor, dims, ""};
    }

    // Model type (user-defined identifier)
    if (check(TokenType::Ident)) {
        auto& tok = advance();
        return TypeInfo{BaseType::Model, {}, tok.lexeme};
    }

    auto& tok = peek();
    throw ParseError("[Parse Error] Line " + std::to_string(tok.loc.line)
        + ", Col " + std::to_string(tok.loc.col)
        + ": Expected type (found '" + tok.lexeme + "')", tok.loc);
}

// ── Statements ──────────────────────────────────────────────────────────────

std::unique_ptr<BlockStmt> Parser::parse_block() {
    SourceLoc loc = peek().loc;
    consume(TokenType::LBrace, "Expected '{'");
    std::vector<StmtPtr> stmts;
    while (!check(TokenType::RBrace) && !is_at_end()) {
        stmts.push_back(parse_stmt());
    }
    consume(TokenType::RBrace, "Expected '}'");
    return std::make_unique<BlockStmt>(std::move(stmts), loc);
}

StmtPtr Parser::parse_stmt() {
    if (check(TokenType::Let))    { advance(); return parse_let_stmt(); }
    if (check(TokenType::If))     { advance(); return parse_if_stmt(); }
    if (check(TokenType::While))  { advance(); return parse_while_stmt(); }
    if (check(TokenType::For))    { advance(); return parse_for_stmt(); }
    if (check(TokenType::Return)) { advance(); return parse_return_stmt(); }
    if (check(TokenType::Print))  { advance(); return parse_print_stmt(); }
    return parse_assign_or_expr_stmt();
}

StmtPtr Parser::parse_let_stmt() {
    SourceLoc loc = previous().loc;
    auto& name_tok = consume(TokenType::Ident, "Expected variable name after 'let'");
    std::string name = name_tok.lexeme;
    consume(TokenType::Colon, "Expected ':' after variable name");
    TypeInfo type = parse_type();
    consume(TokenType::Assign, "Expected '=' in let declaration");
    ExprPtr init = parse_expr();
    consume(TokenType::Semicolon, "Expected ';' after let declaration");
    return std::make_unique<LetStmt>(std::move(name), std::move(type), std::move(init), loc);
}

StmtPtr Parser::parse_assign_or_expr_stmt() {
    // Check for assignment: IDENT = expr ;
    if (check(TokenType::Ident) && peek(1).type == TokenType::Assign) {
        SourceLoc loc = peek().loc;
        std::string name = advance().lexeme;
        advance(); // consume '='
        ExprPtr value = parse_expr();
        consume(TokenType::Semicolon, "Expected ';' after assignment");
        return std::make_unique<AssignStmt>(std::move(name), std::move(value), loc);
    }
    // Expression statement
    SourceLoc loc = peek().loc;
    ExprPtr expr = parse_expr();
    consume(TokenType::Semicolon, "Expected ';' after expression statement");
    return std::make_unique<ExprStmt>(std::move(expr), loc);
}

StmtPtr Parser::parse_if_stmt() {
    SourceLoc loc = previous().loc;
    consume(TokenType::LParen, "Expected '(' after 'if'");
    ExprPtr condition = parse_expr();
    consume(TokenType::RParen, "Expected ')' after if condition");
    auto then_branch = parse_block();
    std::unique_ptr<BlockStmt> else_branch = nullptr;
    if (match(TokenType::Else)) {
        if (match(TokenType::If)) {
            auto nested_if = parse_if_stmt();
            SourceLoc else_loc = nested_if->loc;
            std::vector<StmtPtr> stmts;
            stmts.push_back(std::move(nested_if));
            else_branch = std::make_unique<BlockStmt>(std::move(stmts), else_loc);
        } else {
            else_branch = parse_block();
        }
    }
    return std::make_unique<IfStmt>(std::move(condition), std::move(then_branch), std::move(else_branch), loc);
}

StmtPtr Parser::parse_while_stmt() {
    SourceLoc loc = previous().loc;
    consume(TokenType::LParen, "Expected '(' after 'while'");
    ExprPtr condition = parse_expr();
    consume(TokenType::RParen, "Expected ')' after while condition");
    auto body = parse_block();
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body), loc);
}

StmtPtr Parser::parse_for_stmt() {
    SourceLoc loc = previous().loc;
    consume(TokenType::LParen, "Expected '(' after 'for'");
    auto& var_tok = consume(TokenType::Ident, "Expected loop variable name");
    std::string loop_var = var_tok.lexeme;
    consume(TokenType::In, "Expected 'in' in for loop");
    ExprPtr start_expr = parse_expr();
    consume(TokenType::DotDot, "Expected '..' in for loop range");
    ExprPtr end_expr = parse_expr();
    consume(TokenType::RParen, "Expected ')' after for loop range");
    auto body = parse_block();
    return std::make_unique<ForStmt>(std::move(loop_var), std::move(start_expr), std::move(end_expr), std::move(body), loc);
}

StmtPtr Parser::parse_return_stmt() {
    SourceLoc loc = previous().loc;
    ExprPtr value = nullptr;
    if (!check(TokenType::Semicolon)) {
        value = parse_expr();
    }
    consume(TokenType::Semicolon, "Expected ';' after return statement");
    return std::make_unique<ReturnStmt>(std::move(value), loc);
}

StmtPtr Parser::parse_print_stmt() {
    SourceLoc loc = previous().loc;
    consume(TokenType::LParen, "Expected '(' after 'print'");
    ExprPtr expr = parse_expr();
    consume(TokenType::RParen, "Expected ')' after print argument");
    consume(TokenType::Semicolon, "Expected ';' after print statement");
    return std::make_unique<PrintStmt>(std::move(expr), loc);
}

// ── Expressions ─────────────────────────────────────────────────────────────

ExprPtr Parser::parse_expr() {
    return parse_logic_or();
}

ExprPtr Parser::parse_logic_or() {
    auto left = parse_logic_and();
    while (match(TokenType::Or)) {
        SourceLoc loc = previous().loc;
        auto right = parse_logic_and();
        left = std::make_unique<BinaryExpr>(TokenType::Or, std::move(left), std::move(right), loc);
    }
    return left;
}

ExprPtr Parser::parse_logic_and() {
    auto left = parse_equality();
    while (match(TokenType::And)) {
        SourceLoc loc = previous().loc;
        auto right = parse_equality();
        left = std::make_unique<BinaryExpr>(TokenType::And, std::move(left), std::move(right), loc);
    }
    return left;
}

ExprPtr Parser::parse_equality() {
    auto left = parse_comparison();
    while (check(TokenType::Eq) || check(TokenType::Neq)) {
        TokenType op = advance().type;
        SourceLoc loc = previous().loc;
        auto right = parse_comparison();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right), loc);
    }
    return left;
}

ExprPtr Parser::parse_comparison() {
    auto left = parse_term();
    while (check(TokenType::Lt) || check(TokenType::Leq) ||
           check(TokenType::Gt) || check(TokenType::Geq)) {
        TokenType op = advance().type;
        SourceLoc loc = previous().loc;
        auto right = parse_term();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right), loc);
    }
    return left;
}

ExprPtr Parser::parse_term() {
    auto left = parse_factor();
    while (check(TokenType::Plus) || check(TokenType::Minus)) {
        TokenType op = advance().type;
        SourceLoc loc = previous().loc;
        auto right = parse_factor();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right), loc);
    }
    return left;
}

ExprPtr Parser::parse_factor() {
    auto left = parse_unary();
    while (check(TokenType::Star) || check(TokenType::Slash) ||
           check(TokenType::Percent) || check(TokenType::MatMul)) {
        TokenType op = advance().type;
        SourceLoc loc = previous().loc;
        auto right = parse_unary();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right), loc);
    }
    return left;
}

ExprPtr Parser::parse_unary() {
    if (check(TokenType::Minus) || check(TokenType::Not)) {
        TokenType op = advance().type;
        SourceLoc loc = previous().loc;
        auto operand = parse_unary();
        return std::make_unique<UnaryExpr>(op, std::move(operand), loc);
    }
    return parse_postfix();
}

ExprPtr Parser::parse_postfix() {
    auto expr = parse_primary();

    while (true) {
        if (match(TokenType::Dot)) {
            // Member call: expr.method(args)
            auto& method_tok = consume(TokenType::Ident, "Expected method name after '.'");
            std::string method = method_tok.lexeme;
            consume(TokenType::LParen, "Expected '(' after method name");
            std::vector<ExprPtr> args;
            if (!check(TokenType::RParen)) {
                args.push_back(parse_expr());
                while (match(TokenType::Comma)) {
                    args.push_back(parse_expr());
                }
            }
            SourceLoc loc = method_tok.loc;
            consume(TokenType::RParen, "Expected ')' after method arguments");
            expr = std::make_unique<MemberCallExpr>(std::move(expr), std::move(method), std::move(args), loc);
        } else if (match(TokenType::LParen)) {
            // Function / constructor call: ident(args)
            // The expr should be an IdentExpr
            auto* ident = dynamic_cast<IdentExpr*>(expr.get());
            if (!ident) {
                throw ParseError("[Parse Error] Line " + std::to_string(previous().loc.line)
                    + ", Col " + std::to_string(previous().loc.col)
                    + ": Expected function or constructor name before '('", previous().loc);
            }
            std::string callee = ident->name;
            SourceLoc loc = ident->loc;
            std::vector<ExprPtr> args;
            if (!check(TokenType::RParen)) {
                args.push_back(parse_expr());
                while (match(TokenType::Comma)) {
                    args.push_back(parse_expr());
                }
            }
            consume(TokenType::RParen, "Expected ')' after arguments");
            expr = std::make_unique<CallExpr>(std::move(callee), std::move(args), loc);
        } else {
            break;
        }
    }

    return expr;
}

ExprPtr Parser::parse_primary() {
    SourceLoc loc = peek().loc;

    // Integer literal
    if (match(TokenType::IntLit)) {
        return std::make_unique<IntLitExpr>(std::stoll(previous().lexeme), previous().loc);
    }
    // Float literal
    if (match(TokenType::FloatLit)) {
        return std::make_unique<FloatLitExpr>(std::stof(previous().lexeme), previous().loc);
    }
    // String literal
    if (match(TokenType::StringLit)) {
        return std::make_unique<StringLitExpr>(previous().lexeme, previous().loc);
    }
    // Boolean literals
    if (match(TokenType::True)) {
        return std::make_unique<BoolLitExpr>(true, previous().loc);
    }
    if (match(TokenType::False)) {
        return std::make_unique<BoolLitExpr>(false, previous().loc);
    }

    // Tensor initializers: zeros(R, C), ones(R, C), rand(R, C)
    if (check(TokenType::Zeros) || check(TokenType::Ones) || check(TokenType::Rand)) {
        auto& kind_tok = advance();
        std::string kind = kind_tok.lexeme;
        SourceLoc kloc = kind_tok.loc;
        consume(TokenType::LParen, "Expected '(' after '" + kind + "'");
        auto& rows_tok = consume(TokenType::IntLit, "Expected integer for rows");
        int rows = std::stoi(rows_tok.lexeme);
        consume(TokenType::Comma, "Expected ',' between rows and cols");
        auto& cols_tok = consume(TokenType::IntLit, "Expected integer for cols");
        int cols = std::stoi(cols_tok.lexeme);
        consume(TokenType::RParen, "Expected ')' after cols");
        return std::make_unique<TensorInitExpr>(std::move(kind), rows, cols, kloc);
    }

    // Tensor literal: [ ... ]
    if (check(TokenType::LBracket)) {
        return parse_tensor_lit();
    }

    // Parenthesized expression
    if (match(TokenType::LParen)) {
        auto expr = parse_expr();
        consume(TokenType::RParen, "Expected ')' after expression");
        return expr;
    }

    // Identifier
    if (match(TokenType::Ident)) {
        return std::make_unique<IdentExpr>(previous().lexeme, previous().loc);
    }

    throw ParseError("[Parse Error] Line " + std::to_string(loc.line)
        + ", Col " + std::to_string(loc.col)
        + ": Expected expression (found '" + peek().lexeme + "')", loc);
}

ExprPtr Parser::parse_tensor_lit() {
    SourceLoc loc = peek().loc;
    consume(TokenType::LBracket, "Expected '['");
    std::vector<ExprPtr> elements;

    if (!check(TokenType::RBracket)) {
        elements.push_back(parse_expr());
        while (match(TokenType::Comma)) {
            elements.push_back(parse_expr());
        }
    }

    consume(TokenType::RBracket, "Expected ']' after tensor literal");
    return std::make_unique<TensorLitExpr>(std::move(elements), loc);
}

} // namespace exatron

