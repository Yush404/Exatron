#include "type_checker.hpp"
#include <algorithm>
#include <unordered_set>

namespace exatron {

// Scope Management

void TypeChecker::push_scope() {
    scopes_.emplace_back();
}

void TypeChecker::pop_scope() {
    scopes_.pop_back();
}

void TypeChecker::declare_var(const std::string& name, const TypeInfo& type, SourceLoc loc) {
    auto& scope = scopes_.back();
    if (scope.find(name) != scope.end()) {
        throw TypeError("[Type Error] Line " + std::to_string(loc.line)
            + ", Col " + std::to_string(loc.col)
            + ": Variable '" + name + "' is already declared in this scope", loc);
    }
    scope[name] = type;
}

TypeInfo TypeChecker::lookup_var(const std::string& name, SourceLoc loc) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second;
    }
    if (functions_.find(name) != functions_.end()) {
        throw TypeError("[Type Error] Line " + std::to_string(loc.line)
            + ", Col " + std::to_string(loc.col)
            + ": Function '" + name + "' cannot be used as a value", loc);
    }
    throw TypeError("[Type Error] Line " + std::to_string(loc.line)
        + ", Col " + std::to_string(loc.col)
        + ": Undefined variable '" + name + "'", loc);
}

void TypeChecker::validate_type(const TypeInfo& type, SourceLoc loc) {
    if (type.base == BaseType::Model) {
        if (models_.find(type.model_name) == models_.end()) {
            throw TypeError("[Type Error] Line " + std::to_string(loc.line)
                + ", Col " + std::to_string(loc.col)
                + ": Unknown type '" + type.model_name + "'", loc);
        }
    }
}

bool TypeChecker::block_always_returns(const BlockStmt& block) const {
    if (block.statements.empty()) return false;
    return stmt_always_returns(*block.statements.back());
}

bool TypeChecker::stmt_always_returns(const Stmt& stmt) const {
    if (dynamic_cast<const ReturnStmt*>(&stmt)) return true;
    if (const auto* if_stmt = dynamic_cast<const IfStmt*>(&stmt)) {
        if (!if_stmt->else_branch) return false;
        return block_always_returns(*if_stmt->then_branch) &&
               block_always_returns(*if_stmt->else_branch);
    }
    return false;
}

// check_program

void TypeChecker::check_program(Program& program) {
    // Pass 1: Register and check models
    for (auto& model : program.models) {
        if (models_.find(model.name) != models_.end()) {
            throw TypeError("[Type Error] Line " + std::to_string(model.loc.line)
                + ", Col " + std::to_string(model.loc.col)
                + ": Duplicate model '" + model.name + "'", model.loc);
        }
        check_model(model);
    }

    // Pass 1b: Register function signatures and validate parameter/return types
    for (auto& fn : program.functions) {
        if (functions_.find(fn.name) != functions_.end()) {
            throw TypeError("[Type Error] Line " + std::to_string(fn.loc.line)
                + ", Col " + std::to_string(fn.loc.col)
                + ": Duplicate function '" + fn.name + "'", fn.loc);
        }
        validate_type(fn.return_type, fn.loc);
        FnSignature sig;
        std::unordered_set<std::string> param_names;
        for (auto& p : fn.params) {
            if (param_names.find(p.name) != param_names.end()) {
                throw TypeError("[Type Error] Line " + std::to_string(p.loc.line)
                    + ", Col " + std::to_string(p.loc.col)
                    + ": Duplicate parameter '" + p.name + "' in function '" + fn.name + "'", p.loc);
            }
            param_names.insert(p.name);
            validate_type(p.type, p.loc);
            sig.param_types.push_back(p.type);
        }
        sig.return_type = fn.return_type;
        functions_[fn.name] = sig;
    }

    // Verify main exists with correct signature
    auto main_it = functions_.find("main");
    if (main_it == functions_.end()) {
        throw TypeError("[Type Error] Line 1, Col 1: No 'main' function found", {1, 1});
    }
    if (!main_it->second.param_types.empty() || main_it->second.return_type.base != BaseType::Int) {
        // Find the main function location
        SourceLoc mloc{1,1};
        for (auto& fn : program.functions) {
            if (fn.name == "main") { mloc = fn.loc; break; }
        }
        throw TypeError("[Type Error] Line " + std::to_string(mloc.line)
            + ", Col " + std::to_string(mloc.col)
            + ": 'main' must have signature fn main() -> int", mloc);
    }

    // Pass 2: Check function bodies
    for (auto& fn : program.functions) {
        check_fn(fn);
    }
}

// check_model

void TypeChecker::check_model(ModelDecl& model) {
    if (model.input_type.base != BaseType::Tensor) {
        throw TypeError("[Type Error] Line " + std::to_string(model.loc.line)
            + ", Col " + std::to_string(model.loc.col)
            + ": Model input must be a tensor type", model.loc);
    }
    // Normalize 1D {N} to 2D {1, N}
    if (model.input_type.shape.size() == 1) {
        model.input_type.shape.insert(model.input_type.shape.begin(), 1);
    }

    int current_features = model.input_type.shape[1];
    std::unordered_set<std::string> layer_names;

    for (auto& layer : model.layers) {
        if (layer_names.find(layer.name) != layer_names.end()) {
            throw TypeError("[Type Error] Line " + std::to_string(layer.loc.line)
                + ", Col " + std::to_string(layer.loc.col)
                + ": Duplicate layer '" + layer.name + "' in model '" + model.name + "'", layer.loc);
        }
        layer_names.insert(layer.name);

        if (layer.in_features != current_features) {
            throw TypeError("[Type Error] Line " + std::to_string(layer.loc.line)
                + ", Col " + std::to_string(layer.loc.col)
                + ": Layer '" + layer.name + "' expects input dimension "
                + std::to_string(layer.in_features)
                + ", but previous dimension is " + std::to_string(current_features), layer.loc);
        }
        current_features = layer.out_features;
    }

    model.output_type = TypeInfo{BaseType::Tensor, {model.input_type.shape[0], current_features}, ""};

    models_[model.name] = ModelSignature{model.name, model.input_type, model.output_type};
}

// check_fn

void TypeChecker::check_fn(FnDecl& fn) {
    current_return_type_ = fn.return_type;
    push_scope();
    for (auto& p : fn.params) {
        // Normalize 1D tensor params
        if (p.type.base == BaseType::Tensor && p.type.shape.size() == 1) {
            p.type.shape.insert(p.type.shape.begin(), 1);
        }
        declare_var(p.name, p.type, p.loc);
    }
    check_block(*fn.body);
    pop_scope();

    if (fn.return_type.base != BaseType::Void) {
        if (!block_always_returns(*fn.body)) {
            throw TypeError("[Type Error] Line " + std::to_string(fn.loc.line)
                + ", Col " + std::to_string(fn.loc.col)
                + ": Function '" + fn.name + "' might not return a value on all paths", fn.loc);
        }
    }
}

// ── check_block ─────────────────────────────────────────────────────────────

void TypeChecker::check_block(BlockStmt& block) {
    for (auto& stmt : block.statements) {
        check_stmt(*stmt);
    }
}

// ── check_stmt ──────────────────────────────────────────────────────────────

void TypeChecker::check_stmt(Stmt& stmt) {
    if (auto* s = dynamic_cast<LetStmt*>(&stmt)) {
        // Normalize 1D tensor<N> to {1, N}
        if (s->declared_type.base == BaseType::Tensor && s->declared_type.shape.size() == 1) {
            s->declared_type.shape.insert(s->declared_type.shape.begin(), 1);
        }
        validate_type(s->declared_type, s->loc);
        TypeInfo init_type = check_expr(*s->init);

        // Allow model constructor assignment: let net: ModelName = ModelName();
        if (s->declared_type.base == BaseType::Model && init_type.base == BaseType::Model) {
            if (s->declared_type.model_name != init_type.model_name) {
                throw TypeError("[Type Error] Line " + std::to_string(s->loc.line)
                    + ", Col " + std::to_string(s->loc.col)
                    + ": Type mismatch: declared " + s->declared_type.to_string()
                    + " but got " + init_type.to_string(), s->loc);
            }
        } else if (init_type != s->declared_type) {
            throw TypeError("[Type Error] Line " + std::to_string(s->loc.line)
                + ", Col " + std::to_string(s->loc.col)
                + ": Type mismatch in let declaration: declared "
                + s->declared_type.to_string() + " but initializer has type "
                + init_type.to_string(), s->loc);
        }
        declare_var(s->name, s->declared_type, s->loc);
        return;
    }

    if (auto* s = dynamic_cast<AssignStmt*>(&stmt)) {
        TypeInfo var_type = lookup_var(s->name, s->loc);
        TypeInfo rhs_type = check_expr(*s->value);
        if (rhs_type != var_type) {
            throw TypeError("[Type Error] Line " + std::to_string(s->loc.line)
                + ", Col " + std::to_string(s->loc.col)
                + ": Cannot assign " + rhs_type.to_string()
                + " to variable '" + s->name + "' of type " + var_type.to_string(), s->loc);
        }
        return;
    }

    if (auto* s = dynamic_cast<IfStmt*>(&stmt)) {
        TypeInfo cond_type = check_expr(*s->condition);
        if (cond_type.base != BaseType::Bool) {
            throw TypeError("[Type Error] Line " + std::to_string(s->loc.line)
                + ", Col " + std::to_string(s->loc.col)
                + ": If condition must be bool, got " + cond_type.to_string(), s->loc);
        }
        push_scope();
        check_block(*s->then_branch);
        pop_scope();
        if (s->else_branch) {
            push_scope();
            check_block(*s->else_branch);
            pop_scope();
        }
        return;
    }

    if (auto* s = dynamic_cast<WhileStmt*>(&stmt)) {
        TypeInfo cond_type = check_expr(*s->condition);
        if (cond_type.base != BaseType::Bool) {
            throw TypeError("[Type Error] Line " + std::to_string(s->loc.line)
                + ", Col " + std::to_string(s->loc.col)
                + ": While condition must be bool, got " + cond_type.to_string(), s->loc);
        }
        push_scope();
        check_block(*s->body);
        pop_scope();
        return;
    }

    if (auto* s = dynamic_cast<ForStmt*>(&stmt)) {
        TypeInfo start_type = check_expr(*s->start_expr);
        TypeInfo end_type = check_expr(*s->end_expr);
        if (start_type.base != BaseType::Int) {
            throw TypeError("[Type Error] Line " + std::to_string(s->loc.line)
                + ", Col " + std::to_string(s->loc.col)
                + ": For loop start must be int, got " + start_type.to_string(), s->loc);
        }
        if (end_type.base != BaseType::Int) {
            throw TypeError("[Type Error] Line " + std::to_string(s->loc.line)
                + ", Col " + std::to_string(s->loc.col)
                + ": For loop end must be int, got " + end_type.to_string(), s->loc);
        }
        push_scope();
        declare_var(s->loop_var, TypeInfo{BaseType::Int}, s->loc);
        check_block(*s->body);
        pop_scope();
        return;
    }

    if (auto* s = dynamic_cast<ReturnStmt*>(&stmt)) {
        if (s->value) {
            if (current_return_type_.base == BaseType::Void) {
                throw TypeError("[Type Error] Line " + std::to_string(s->loc.line)
                    + ", Col " + std::to_string(s->loc.col)
                    + ": Cannot return a value from a void function", s->loc);
            }
            TypeInfo ret_type = check_expr(*s->value);
            if (ret_type != current_return_type_) {
                throw TypeError("[Type Error] Line " + std::to_string(s->loc.line)
                    + ", Col " + std::to_string(s->loc.col)
                    + ": Return type mismatch: expected " + current_return_type_.to_string()
                    + " but got " + ret_type.to_string(), s->loc);
            }
        } else {
            if (current_return_type_.base != BaseType::Void) {
                throw TypeError("[Type Error] Line " + std::to_string(s->loc.line)
                    + ", Col " + std::to_string(s->loc.col)
                    + ": Non-void function must return a value of type " + current_return_type_.to_string(), s->loc);
            }
        }
        return;
    }

    if (auto* s = dynamic_cast<PrintStmt*>(&stmt)) {
        TypeInfo expr_type = check_expr(*s->expr);
        if (expr_type.base == BaseType::Void || expr_type.base == BaseType::Model) {
            throw TypeError("[Type Error] Line " + std::to_string(s->loc.line)
                + ", Col " + std::to_string(s->loc.col)
                + ": Cannot print value of type " + expr_type.to_string(), s->loc);
        }
        return;
    }

    if (auto* s = dynamic_cast<ExprStmt*>(&stmt)) {
        check_expr(*s->expr);
        return;
    }
}

// ── check_expr ──────────────────────────────────────────────────────────────

TypeInfo TypeChecker::check_expr(Expr& expr) {
    // Integer literal
    if (auto* e = dynamic_cast<IntLitExpr*>(&expr)) {
        e->inferred_type = TypeInfo{BaseType::Int};
        return e->inferred_type;
    }

    // Float literal
    if (auto* e = dynamic_cast<FloatLitExpr*>(&expr)) {
        e->inferred_type = TypeInfo{BaseType::Float};
        return e->inferred_type;
    }

    // Bool literal
    if (auto* e = dynamic_cast<BoolLitExpr*>(&expr)) {
        e->inferred_type = TypeInfo{BaseType::Bool};
        return e->inferred_type;
    }

    // String literal
    if (auto* e = dynamic_cast<StringLitExpr*>(&expr)) {
        e->inferred_type = TypeInfo{BaseType::String};
        return e->inferred_type;
    }

    // Identifier
    if (auto* e = dynamic_cast<IdentExpr*>(&expr)) {
        e->inferred_type = lookup_var(e->name, e->loc);
        return e->inferred_type;
    }

    // Tensor initializer (zeros, ones, rand)
    if (auto* e = dynamic_cast<TensorInitExpr*>(&expr)) {
        if (e->rows <= 0 || e->cols <= 0) {
            throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                + ", Col " + std::to_string(e->loc.col)
                + ": Tensor dimensions must be positive", e->loc);
        }
        e->inferred_type = TypeInfo{BaseType::Tensor, {e->rows, e->cols}, ""};
        return e->inferred_type;
    }

    // Tensor literal
    if (auto* e = dynamic_cast<TensorLitExpr*>(&expr)) {
        if (e->elements.empty()) {
            throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                + ", Col " + std::to_string(e->loc.col)
                + ": Empty tensor literal", e->loc);
        }

        // Check if 2D: first element is a TensorLitExpr (nested array)
        auto* first_inner = dynamic_cast<TensorLitExpr*>(e->elements[0].get());
        if (first_inner) {
            // 2D tensor literal [[...], [...], ...]
            int rows = static_cast<int>(e->elements.size());
            int cols = static_cast<int>(first_inner->elements.size());
            for (auto& row_expr : e->elements) {
                auto* row = dynamic_cast<TensorLitExpr*>(row_expr.get());
                if (!row) {
                    throw TypeError("[Type Error] Line " + std::to_string(row_expr->loc.line)
                        + ", Col " + std::to_string(row_expr->loc.col)
                        + ": Mixed 1D and 2D elements in tensor literal", row_expr->loc);
                }
                if (static_cast<int>(row->elements.size()) != cols) {
                    throw TypeError("[Type Error] Line " + std::to_string(row->loc.line)
                        + ", Col " + std::to_string(row->loc.col)
                        + ": Jagged tensor literal: expected " + std::to_string(cols)
                        + " columns but got " + std::to_string(row->elements.size()), row->loc);
                }
                for (auto& elem : row->elements) {
                    TypeInfo et = check_expr(*elem);
                    if (et.base != BaseType::Float && et.base != BaseType::Int) {
                        throw TypeError("[Type Error] Line " + std::to_string(elem->loc.line)
                            + ", Col " + std::to_string(elem->loc.col)
                            + ": Tensor elements must be numeric", elem->loc);
                    }
                }
            }
            e->inferred_type = TypeInfo{BaseType::Tensor, {rows, cols}, ""};
        } else {
            // 1D tensor literal [1.0, 2.0, ...] -> row vector {1, N}
            int cols = static_cast<int>(e->elements.size());
            for (auto& elem : e->elements) {
                TypeInfo et = check_expr(*elem);
                if (et.base != BaseType::Float && et.base != BaseType::Int) {
                    throw TypeError("[Type Error] Line " + std::to_string(elem->loc.line)
                        + ", Col " + std::to_string(elem->loc.col)
                        + ": Tensor elements must be numeric", elem->loc);
                }
            }
            e->inferred_type = TypeInfo{BaseType::Tensor, {1, cols}, ""};
        }
        return e->inferred_type;
    }

    // Unary expression
    if (auto* e = dynamic_cast<UnaryExpr*>(&expr)) {
        TypeInfo operand_type = check_expr(*e->operand);
        if (e->op == TokenType::Minus) {
            if (operand_type.base != BaseType::Int && operand_type.base != BaseType::Float) {
                throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                    + ", Col " + std::to_string(e->loc.col)
                    + ": Operator '-' cannot be applied to " + operand_type.to_string(), e->loc);
            }
            e->inferred_type = operand_type;
        } else if (e->op == TokenType::Not) {
            if (operand_type.base != BaseType::Bool) {
                throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                    + ", Col " + std::to_string(e->loc.col)
                    + ": Operator '!' cannot be applied to " + operand_type.to_string(), e->loc);
            }
            e->inferred_type = TypeInfo{BaseType::Bool};
        }
        return e->inferred_type;
    }

    // Binary expression
    if (auto* e = dynamic_cast<BinaryExpr*>(&expr)) {
        TypeInfo lhs_type = check_expr(*e->lhs);
        TypeInfo rhs_type = check_expr(*e->rhs);

        switch (e->op) {
            case TokenType::Plus:
            case TokenType::Minus:
            case TokenType::Star: {
                // Tensor element-wise ops
                if (lhs_type.base == BaseType::Tensor && rhs_type.base == BaseType::Tensor) {
                    if (lhs_type.shape != rhs_type.shape) {
                        throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                            + ", Col " + std::to_string(e->loc.col)
                            + ": Element-wise tensor operation requires matching shapes: "
                            + lhs_type.to_string() + " vs " + rhs_type.to_string(), e->loc);
                    }
                    e->inferred_type = lhs_type;
                    return e->inferred_type;
                }
                // Scalar ops
                if (lhs_type.base == rhs_type.base &&
                    (lhs_type.base == BaseType::Int || lhs_type.base == BaseType::Float)) {
                    e->inferred_type = lhs_type;
                    return e->inferred_type;
                }
                throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                    + ", Col " + std::to_string(e->loc.col)
                    + ": Invalid operand types for arithmetic: "
                    + lhs_type.to_string() + " and " + rhs_type.to_string(), e->loc);
            }
            case TokenType::Slash: {
                if (lhs_type.base == rhs_type.base &&
                    (lhs_type.base == BaseType::Int || lhs_type.base == BaseType::Float)) {
                    e->inferred_type = lhs_type;
                    return e->inferred_type;
                }
                throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                    + ", Col " + std::to_string(e->loc.col)
                    + ": Invalid operand types for division: "
                    + lhs_type.to_string() + " and " + rhs_type.to_string(), e->loc);
            }
            case TokenType::Percent: {
                if (lhs_type.base == BaseType::Int && rhs_type.base == BaseType::Int) {
                    e->inferred_type = TypeInfo{BaseType::Int};
                    return e->inferred_type;
                }
                throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                    + ", Col " + std::to_string(e->loc.col)
                    + ": Modulo requires int operands, got "
                    + lhs_type.to_string() + " and " + rhs_type.to_string(), e->loc);
            }
            case TokenType::MatMul: {
                if (lhs_type.base != BaseType::Tensor || rhs_type.base != BaseType::Tensor) {
                    throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                        + ", Col " + std::to_string(e->loc.col)
                        + ": Matrix multiplication '@' requires tensor operands", e->loc);
                }
                if (lhs_type.shape.size() != 2 || rhs_type.shape.size() != 2) {
                    throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                        + ", Col " + std::to_string(e->loc.col)
                        + ": Matrix multiplication '@' requires 2D tensors", e->loc);
                }
                int M = lhs_type.shape[0], K = lhs_type.shape[1];
                int K2 = rhs_type.shape[0], N = rhs_type.shape[1];
                if (K != K2) {
                    throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                        + ", Col " + std::to_string(e->loc.col)
                        + ": Matrix multiplication dimension mismatch: cannot multiply tensor<"
                        + std::to_string(M) + ", " + std::to_string(K) + "> by tensor<"
                        + std::to_string(K2) + ", " + std::to_string(N) + ">", e->loc);
                }
                e->inferred_type = TypeInfo{BaseType::Tensor, {M, N}, ""};
                return e->inferred_type;
            }
            case TokenType::Lt:
            case TokenType::Leq:
            case TokenType::Gt:
            case TokenType::Geq: {
                if (lhs_type.base == rhs_type.base &&
                    (lhs_type.base == BaseType::Int || lhs_type.base == BaseType::Float)) {
                    e->inferred_type = TypeInfo{BaseType::Bool};
                    return e->inferred_type;
                }
                throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                    + ", Col " + std::to_string(e->loc.col)
                    + ": Comparison requires matching int or float operands, got "
                    + lhs_type.to_string() + " and " + rhs_type.to_string(), e->loc);
            }
            case TokenType::Eq:
            case TokenType::Neq: {
                if (lhs_type.base == rhs_type.base &&
                    (lhs_type.base == BaseType::Int || lhs_type.base == BaseType::Float ||
                     lhs_type.base == BaseType::Bool || lhs_type.base == BaseType::String)) {
                    e->inferred_type = TypeInfo{BaseType::Bool};
                    return e->inferred_type;
                }
                throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                    + ", Col " + std::to_string(e->loc.col)
                    + ": Equality comparison requires matching scalar types, got "
                    + lhs_type.to_string() + " and " + rhs_type.to_string(), e->loc);
            }
            case TokenType::And:
            case TokenType::Or: {
                if (lhs_type.base == BaseType::Bool && rhs_type.base == BaseType::Bool) {
                    e->inferred_type = TypeInfo{BaseType::Bool};
                    return e->inferred_type;
                }
                throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                    + ", Col " + std::to_string(e->loc.col)
                    + ": Logical operators require bool operands, got "
                    + lhs_type.to_string() + " and " + rhs_type.to_string(), e->loc);
            }
            default:
                throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                    + ", Col " + std::to_string(e->loc.col)
                    + ": Unknown binary operator", e->loc);
        }
    }

    // Function / model constructor call
    if (auto* e = dynamic_cast<CallExpr*>(&expr)) {
        // Model constructor
        auto model_it = models_.find(e->callee);
        if (model_it != models_.end()) {
            if (!e->args.empty()) {
                throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                    + ", Col " + std::to_string(e->loc.col)
                    + ": Model constructor '" + e->callee + "' takes no arguments", e->loc);
            }
            e->inferred_type = TypeInfo{BaseType::Model, {}, e->callee};
            return e->inferred_type;
        }
        // Function call
        auto fn_it = functions_.find(e->callee);
        if (fn_it != functions_.end()) {
            const auto& sig = fn_it->second;
            if (e->args.size() != sig.param_types.size()) {
                throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                    + ", Col " + std::to_string(e->loc.col)
                    + ": Function '" + e->callee + "' expects "
                    + std::to_string(sig.param_types.size()) + " argument(s) but got "
                    + std::to_string(e->args.size()), e->loc);
            }
            for (std::size_t i = 0; i < e->args.size(); ++i) {
                TypeInfo arg_type = check_expr(*e->args[i]);
                if (arg_type != sig.param_types[i]) {
                    throw TypeError("[Type Error] Line " + std::to_string(e->args[i]->loc.line)
                        + ", Col " + std::to_string(e->args[i]->loc.col)
                        + ": Argument " + std::to_string(i + 1) + " type mismatch: expected "
                        + sig.param_types[i].to_string() + " but got "
                        + arg_type.to_string(), e->args[i]->loc);
                }
            }
            e->inferred_type = sig.return_type;
            return e->inferred_type;
        }
        throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
            + ", Col " + std::to_string(e->loc.col)
            + ": Undefined function '" + e->callee + "'", e->loc);
    }

    // Member call (e.g., net.forward(sample))
    if (auto* e = dynamic_cast<MemberCallExpr*>(&expr)) {
        TypeInfo obj_type = check_expr(*e->object);
        if (obj_type.base != BaseType::Model) {
            throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                + ", Col " + std::to_string(e->loc.col)
                + ": Member call on non-model type " + obj_type.to_string(), e->loc);
        }
        auto model_it = models_.find(obj_type.model_name);
        if (model_it == models_.end()) {
            throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                + ", Col " + std::to_string(e->loc.col)
                + ": Unknown model '" + obj_type.model_name + "'", e->loc);
        }
        if (e->method != "forward") {
            throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                + ", Col " + std::to_string(e->loc.col)
                + ": Model '" + obj_type.model_name + "' has no method '"
                + e->method + "' (only 'forward' is supported)", e->loc);
        }
        if (e->args.size() != 1) {
            throw TypeError("[Type Error] Line " + std::to_string(e->loc.line)
                + ", Col " + std::to_string(e->loc.col)
                + ": forward() expects exactly 1 argument", e->loc);
        }
        TypeInfo arg_type = check_expr(*e->args[0]);
        if (arg_type != model_it->second.input_type) {
            throw TypeError("[Type Error] Line " + std::to_string(e->args[0]->loc.line)
                + ", Col " + std::to_string(e->args[0]->loc.col)
                + ": forward() expects input of type " + model_it->second.input_type.to_string()
                + " but got " + arg_type.to_string(), e->args[0]->loc);
        }
        e->inferred_type = model_it->second.output_type;
        return e->inferred_type;
    }

    throw TypeError("[Type Error] Line " + std::to_string(expr.loc.line)
        + ", Col " + std::to_string(expr.loc.col)
        + ": Unknown expression type", expr.loc);
}

} // namespace exatron

