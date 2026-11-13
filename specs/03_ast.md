# Feature 03: Abstract Syntax Tree (AST) & Type Representation

## 1. Objective
Design and implement a clean, modern C++23 Abstract Syntax Tree (AST) and type representation in `include/ast.hpp`. Every syntactic construct in Exatron—both imperative statements/expressions and neural network `model`/`layer` declarations—must be represented using `std::unique_ptr` nodes with source location tracking and semantic type annotations populated during type checking.

## 2. Type Representation (`TypeInfo`)
In `include/ast.hpp`, define a rich `TypeInfo` structure capable of representing scalars, tensors with static dimensions, and user-defined neural network model types:

```cpp
#pragma once
#include "token.hpp"
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace exatron {

enum class BaseType {
    Int,
    Float,
    Bool,
    String,
    Void,
    Tensor,
    Model
};

struct TypeInfo {
    BaseType base = BaseType::Void;
    std::vector<int> shape{};      // Populated when base == BaseType::Tensor (e.g., {1, 4} or {4, 8})
    std::string model_name{};      // Populated when base == BaseType::Model

    bool operator==(const TypeInfo& other) const {
        return base == other.base && shape == other.shape && model_name == other.model_name;
    }
    bool operator!=(const TypeInfo& other) const {
        return !(*this == other);
    }

    std::string to_string() const {
        switch (base) {
            case BaseType::Int: return "int";
            case BaseType::Float: return "float";
            case BaseType::Bool: return "bool";
            case BaseType::String: return "string";
            case BaseType::Void: return "void";
            case BaseType::Model: return model_name;
            case BaseType::Tensor: {
                std::string s = "tensor<";
                for (std::size_t i = 0; i < shape.size(); ++i) {
                    if (i > 0) s += ", ";
                    s += std::to_string(shape[i]);
                }
                s += ">";
                return s;
            }
        }
        return "unknown";
    }
};
```

## 3. Expression Nodes (`Expr`)
Define a polymorphic base class `Expr` containing `SourceLoc loc` and `TypeInfo inferred_type` (filled by the type checker), along with concrete expression subclasses:

```cpp
struct Expr {
    SourceLoc loc{};
    TypeInfo inferred_type{};
    virtual ~Expr() = default;
};
using ExprPtr = std::unique_ptr<Expr>;

struct IntLitExpr : Expr {
    long long value;
    IntLitExpr(long long v, SourceLoc l) : value(v) { loc = l; }
};

struct FloatLitExpr : Expr {
    float value;
    FloatLitExpr(float v, SourceLoc l) : value(v) { loc = l; }
};

struct BoolLitExpr : Expr {
    bool value;
    BoolLitExpr(bool v, SourceLoc l) : value(v) { loc = l; }
};

struct StringLitExpr : Expr {
    std::string value;
    StringLitExpr(std::string v, SourceLoc l) : value(std::move(v)) { loc = l; }
};

struct IdentExpr : Expr {
    std::string name;
    IdentExpr(std::string n, SourceLoc l) : name(std::move(n)) { loc = l; }
};

// Supports 1D [1.0, 2.0] and 2D [[1.0, 2.0], [3.0, 4.0]] tensor literals
struct TensorLitExpr : Expr {
    std::vector<ExprPtr> elements;
    TensorLitExpr(std::vector<ExprPtr> elems, SourceLoc l)
        : elements(std::move(elems)) { loc = l; }
};

// Built-in tensor initializers: zeros(R, C), ones(R, C), rand(R, C)
struct TensorInitExpr : Expr {
    std::string kind; // "zeros", "ones", or "rand"
    int rows;
    int cols;
    TensorInitExpr(std::string k, int r, int c, SourceLoc l)
        : kind(std::move(k)), rows(r), cols(c) { loc = l; }
};

struct UnaryExpr : Expr {
    TokenType op; // Minus or Not
    ExprPtr operand;
    UnaryExpr(TokenType o, ExprPtr expr, SourceLoc l)
        : op(o), operand(std::move(expr)) { loc = l; }
};

struct BinaryExpr : Expr {
    TokenType op; // +, -, *, /, %, @, ==, !=, <, <=, >, >=, &&, ||
    ExprPtr lhs;
    ExprPtr rhs;
    BinaryExpr(TokenType o, ExprPtr l_expr, ExprPtr r_expr, SourceLoc l)
        : op(o), lhs(std::move(l_expr)), rhs(std::move(r_expr)) { loc = l; }
};

struct CallExpr : Expr {
    std::string callee;
    std::vector<ExprPtr> args;
    CallExpr(std::string c, std::vector<ExprPtr> a, SourceLoc l)
        : callee(std::move(c)), args(std::move(a)) { loc = l; }
};

// Model method call: e.g., net.forward(sample)
struct MemberCallExpr : Expr {
    ExprPtr object;
    std::string method;
    std::vector<ExprPtr> args;
    MemberCallExpr(ExprPtr obj, std::string m, std::vector<ExprPtr> a, SourceLoc l)
        : object(std::move(obj)), method(std::move(m)), args(std::move(a)) { loc = l; }
};
```

## 4. Statement Nodes (`Stmt`)
Define the polymorphic base class `Stmt` and concrete statement subclasses:

```cpp
struct Stmt {
    SourceLoc loc{};
    virtual ~Stmt() = default;
};
using StmtPtr = std::unique_ptr<Stmt>;

struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;
    BlockStmt(std::vector<StmtPtr> stmts, SourceLoc l)
        : statements(std::move(stmts)) { loc = l; }
};

struct LetStmt : Stmt {
    std::string name;
    TypeInfo declared_type;
    ExprPtr init;
    LetStmt(std::string n, TypeInfo t, ExprPtr i, SourceLoc l)
        : name(std::move(n)), declared_type(std::move(t)), init(std::move(i)) { loc = l; }
};

struct AssignStmt : Stmt {
    std::string name;
    ExprPtr value;
    AssignStmt(std::string n, ExprPtr v, SourceLoc l)
        : name(std::move(n)), value(std::move(v)) { loc = l; }
};

struct IfStmt : Stmt {
    ExprPtr condition;
     std::unique_ptr<BlockStmt> then_branch;
    std::unique_ptr<BlockStmt> else_branch; // nullptr if no else
    IfStmt(ExprPtr cond, std::unique_ptr<BlockStmt> then_b, std::unique_ptr<BlockStmt> else_b, SourceLoc l)
        : condition(std::move(cond)), then_branch(std::move(then_b)), else_branch(std::move(else_b)) { loc = l; }
};

struct WhileStmt : Stmt {
    ExprPtr condition;
    std::unique_ptr<BlockStmt> body;
    WhileStmt(ExprPtr cond, std::unique_ptr<BlockStmt> b, SourceLoc l)
        : condition(std::move(cond)), body(std::move(b)) { loc = l; }
};

struct ForStmt : Stmt {
    std::string loop_var;
    ExprPtr start_expr;
    ExprPtr end_expr;
    std::unique_ptr<BlockStmt> body;
    ForStmt(std::string var, ExprPtr s, ExprPtr e, std::unique_ptr<BlockStmt> b, SourceLoc l)
        : loop_var(std::move(var)), start_expr(std::move(s)), end_expr(std::move(e)), body(std::move(b)) { loc = l; }
};

struct ReturnStmt : Stmt {
    ExprPtr value; // nullptr for void return
    ReturnStmt(ExprPtr v, SourceLoc l) : value(std::move(v)) { loc = l; }
};

struct PrintStmt : Stmt {
    ExprPtr expr;
    PrintStmt(ExprPtr e, SourceLoc l) : expr(std::move(e)) { loc = l; }
};

struct ExprStmt : Stmt {
    ExprPtr expr;
    ExprStmt(ExprPtr e, SourceLoc l) : expr(std::move(e)) { loc = l; }
};
```

## 5. Top-Level Declarations (`FnDecl`, `ModelDecl`, `Program`)
Define top-level AST structures for functions and neural network models:

```cpp
struct Param {
    std::string name;
    TypeInfo type;
    SourceLoc loc{};
};

struct FnDecl {
    std::string name;
    std::vector<Param> params;
    TypeInfo return_type;
    std::unique_ptr<BlockStmt> body;
    SourceLoc loc{};
};

enum class ActivationKind {
    Relu,
    Sigmoid,
    Tanh,
    Softmax,
    Linear
};

struct LayerDecl {
    std::string name;
    int in_features;
    int out_features;
    ActivationKind activation;
    SourceLoc loc{};
};

struct ModelDecl {
    std::string name;
    TypeInfo input_type;       // e.g., tensor<1, 4>
    TypeInfo output_type;      // Inferred by type checker from last layer: e.g., tensor<1, out_features>
    std::vector<LayerDecl> layers;
    SourceLoc loc{};
};

struct Program {
    std::vector<ModelDecl> models;
    std::vector<FnDecl> functions;
};

} // namespace exatron
```

## 6. Verification Step
Ensure `include/ast.hpp` compiles without warnings under `-std=c++23 -Wall -Wextra -Wpedantic` when included in `src/main.cpp`.
