# Feature 04: Recursive Descent Parser

## 1. Objective
Implement a hand-written recursive descent parser in `include/parser.hpp` and `src/parser.cpp` that consumes the `std::vector<Token>` produced by the `Lexer` and constructs a complete `exatron::Program` AST.

## 2. Parser Interface (`include/parser.hpp`)
Create `include/parser.hpp`:

```cpp
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
```

## 3. Detailed Parsing Rules (`src/parser.cpp`)

### 3.1 Top-Level Parsing (`parse_program`)
Loop until `is_at_end()`:
- If `match(TokenType::Model)`, call `parse_model_decl()` and append to `program.models`.
- Else if `match(TokenType::Fn)`, call `parse_fn_decl()` and append to `program.functions`.
- Else throw `ParseError` at `peek().loc` stating `"Expected 'model' or 'fn' declaration at top level"`.

### 3.2 Neural Network Model Parsing (`parse_model_decl`)
Syntax:
```text
model Classifier {
    input: tensor<1, 4>;
    layer fc1: Dense(4, 8, relu);
    layer fc2: Dense(8, 2, softmax);
}
```
1. Consume `TokenType::Ident` for the model name.
2. Consume `TokenType::LBrace`.
3. Consume `TokenType::Input`, `TokenType::Colon`, then call `parse_type()` to obtain the input tensor type, and consume `TokenType::Semicolon`.
4. Parse one or more `layer` declarations (`while (match(TokenType::Layer))`):
   - Consume `TokenType::Ident` (layer name).
   - Consume `TokenType::Colon`.
   - Consume `TokenType::Dense`, then `TokenType::LParen`.
   - Consume `TokenType::IntLit` (`in_features`), `TokenType::Comma`, `TokenType::IntLit` (`out_features`), `TokenType::Comma`.
   - Call `parse_activation()` which accepts `Relu`, `Sigmoid`, `Tanh`, `Softmax`, or `Linear`.
   - Consume `TokenType::RParen` and `TokenType::Semicolon`.
5. Consume `TokenType::RBrace`. If `layers` is empty, throw `ParseError("Model must contain at least one layer")`.

### 3.3 Type Annotations (`parse_type`)
- `IntType` -> `TypeInfo{BaseType::Int}`
- `FloatType` -> `TypeInfo{BaseType::Float}`
- `BoolType` -> `TypeInfo{BaseType::Bool}`
- `StringType` -> `TypeInfo{BaseType::String}`
- `VoidType` -> `TypeInfo{BaseType::Void}`
- `TensorType` -> Consume `TokenType::Lt`, parse comma-separated `TokenType::IntLit` dimensions (converting each to `int` and ensuring `> 0`), consume `TokenType::Gt`, and return `TypeInfo{BaseType::Tensor, dims, ""}`. Normalize 1D `tensor<N>` to 2D row-vector shape `{1, N}` or preserve `{N}` consistently (recommended: normalize 1D `tensor<N>` to `{1, N}` or support 1D/2D explicitly; simplest and cleanest for matrix multiplication is treating `tensor<R, C>` as 2D `{R, C}` and `tensor<N>` as `{1, N}` row vector).
- `Ident` -> Return `TypeInfo{BaseType::Model, {}, ident_lexeme}` so variables can be declared with model types like `let net: Classifier = Classifier();`.

### 3.4 Statements (`parse_stmt`)
- `let`: `"let" IDENT ":" Type "=" Expr ";"` -> `LetStmt`.
- `if`: `"if" "(" Expr ")" Block ("else" Block)?` -> `IfStmt`.
- `while`: `"while" "(" Expr ")" Block` -> `WhileStmt`.
- `for`: `"for" "(" IDENT "in" Expr ".." Expr ")" Block` -> `ForStmt`.
- `return`: `"return" Expr? ";"` -> `ReturnStmt` (check if next token is `;` for void return).
- `print`: `"print" "(" Expr ")" ";"` -> `PrintStmt`.
- Assignment vs. Expression Statement (`parse_assign_or_expr_stmt`):
  - If `check(TokenType::Ident) && peek(1).type == TokenType::Assign`, consume `Ident`, consume `Assign`, parse `Expr`, consume `Semicolon`, and return `AssignStmt`.
  - Otherwise parse `Expr`, consume `Semicolon`, and return `ExprStmt`.

### 3.5 Expressions & Tensor Literals
- Follow the exact operator precedence hierarchy from Feature 01 (`||` -> `&&` -> `== !=` -> `< <= > >=` -> `+ -` -> `* / % @` -> unary `- !` -> postfix `.` and `()`).
- In `parse_primary()`:
  - Handle `Zeros`, `Ones`, `Rand` keywords followed by `(` `IntLit` `,` `IntLit` `)` to construct `TensorInitExpr(kind, rows, cols, loc)`.
  - Handle `[` by calling `parse_tensor_lit()`, which parses comma-separated expressions up to `]` and returns `TensorLitExpr`.
  - In `parse_postfix()`, after parsing a `Primary`, loop while `match(TokenType::Dot)` or `match(TokenType::LParen)`:
    - If `Dot`: consume method name (`Ident`), consume `(`, parse optional argument list, consume `)`, and wrap in `MemberCallExpr`.
    - If `LParen`: require that the primary was an `IdentExpr` (the function or model constructor name), parse optional argument list, consume `)`, and return `CallExpr`.

## 4. Error Formatting
Every `ParseError` must include the line and column from the offending token:
`"[Parse Error] Line <line>, Col <col>: <message> (found '<lexeme>')"`

## 5. Verification Step
Compile with `cmake --build build` and verify that sample programs containing functions, loops, tensor literals, and `model` blocks parse into an AST without errors.
