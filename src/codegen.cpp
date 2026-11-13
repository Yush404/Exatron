#include "codegen.hpp"
#include "runtime_emitter.hpp"
#include <iomanip>

namespace exatron {

// Helpers

void CodeGenerator::emit_indent() {
    for (int i = 0; i < indent_level_; ++i) out_ << "    ";
}

std::string CodeGenerator::mangle(const std::string& name) const {
    if (name == "main") return name;
    return "ex_" + name;
}

std::string CodeGenerator::map_type(const TypeInfo& type) const {
    switch (type.base) {
        case BaseType::Int:    return "int64_t";
        case BaseType::Float:  return "float";
        case BaseType::Bool:   return "bool";
        case BaseType::String: return "std::string";
        case BaseType::Void:   return "void";
        case BaseType::Tensor: return "exatron_rt::Tensor";
        case BaseType::Model:  return mangle(type.model_name);
    }
    return "void";
}

// generate

std::string CodeGenerator::generate(const Program& program) {
    out_.str("");
    out_.clear();
    loop_counter_ = 0;

    // Emit runtime header
    out_ << EXATRON_RUNTIME_HEADER << "\n\n";

    // Emit model structs
    for (const auto& model : program.models) {
        emit_model(model);
        out_ << "\n";
    }

    // Forward-declare all functions except main (for recursion / out-of-order calls)
    for (const auto& fn : program.functions) {
        if (fn.name == "main") continue;
        out_ << map_type(fn.return_type) << " " << mangle(fn.name) << "(";
        for (std::size_t i = 0; i < fn.params.size(); ++i) {
            if (i > 0) out_ << ", ";
            out_ << map_type(fn.params[i].type) << " " << mangle(fn.params[i].name);
        }
        out_ << ");\n";
    }
    if (program.functions.size() > 1) out_ << "\n";

    // Emit function definitions in source order
    for (const auto& fn : program.functions) {
        emit_fn_decl(fn);
        out_ << "\n";
    }

    return out_.str();
}

// emit_model

static std::string activation_to_fn(ActivationKind act) {
    switch (act) {
        case ActivationKind::Relu:    return "exatron_rt::relu";
        case ActivationKind::Sigmoid: return "exatron_rt::sigmoid";
        case ActivationKind::Tanh:    return "exatron_rt::tanh_act";
        case ActivationKind::Softmax: return "exatron_rt::softmax";
        case ActivationKind::Linear:  return "exatron_rt::linear";
    }
    return "exatron_rt::linear";
}

void CodeGenerator::emit_model(const ModelDecl& model) {
    out_ << "struct " << mangle(model.name) << " {\n";
    indent_level_++;

    // Layer fields
    for (const auto& layer : model.layers) {
        emit_indent();
        out_ << "exatron_rt::DenseLayer " << mangle(layer.name)
             << "{" << layer.in_features << ", " << layer.out_features << "};\n";
    }
    out_ << "\n";

    // Forward method
    emit_indent();
    out_ << "exatron_rt::Tensor forward(const exatron_rt::Tensor& input) const {\n";
    indent_level_++;
    emit_indent();
    out_ << "exatron_rt::Tensor x = input;\n";
    for (const auto& layer : model.layers) {
        emit_indent();
        out_ << "x = " << activation_to_fn(layer.activation)
             << "(" << mangle(layer.name) << ".linear_step(x));\n";
    }
    emit_indent();
    out_ << "return x;\n";
    indent_level_--;
    emit_indent();
    out_ << "}\n";

    indent_level_--;
    out_ << "};\n";
}

// emit_fn_decl

void CodeGenerator::emit_fn_decl(const FnDecl& fn) {
    if (fn.name == "main") {
        out_ << "int main(";
    } else {
        out_ << map_type(fn.return_type) << " " << mangle(fn.name) << "(";
    }
    for (std::size_t i = 0; i < fn.params.size(); ++i) {
        if (i > 0) out_ << ", ";
        out_ << map_type(fn.params[i].type) << " " << mangle(fn.params[i].name);
    }
    out_ << ") ";
    emit_block(*fn.body, true);
    out_ << "\n";
}

// emit_block

void CodeGenerator::emit_block(const BlockStmt& block, bool include_braces) {
    if (include_braces) {
        out_ << "{\n";
        indent_level_++;
    }
    for (const auto& stmt : block.statements) {
        emit_stmt(*stmt);
    }
    if (include_braces) {
        indent_level_--;
        emit_indent();
        out_ << "}";
    }
}

// emit_stmt

void CodeGenerator::emit_stmt(const Stmt& stmt) {
    if (const auto* s = dynamic_cast<const LetStmt*>(&stmt)) {
        emit_indent();
        out_ << map_type(s->declared_type) << " " << mangle(s->name) << " = ";
        emit_expr(*s->init);
        out_ << ";\n";
        return;
    }

    if (const auto* s = dynamic_cast<const AssignStmt*>(&stmt)) {
        emit_indent();
        out_ << mangle(s->name) << " = ";
        emit_expr(*s->value);
        out_ << ";\n";
        return;
    }

    if (const auto* s = dynamic_cast<const IfStmt*>(&stmt)) {
        emit_indent();
        out_ << "if (";
        emit_expr(*s->condition);
        out_ << ") ";
        emit_block(*s->then_branch, true);
        if (s->else_branch) {
            out_ << " else ";
            emit_block(*s->else_branch, true);
        }
        out_ << "\n";
        return;
    }

    if (const auto* s = dynamic_cast<const WhileStmt*>(&stmt)) {
        emit_indent();
        out_ << "while (";
        emit_expr(*s->condition);
        out_ << ") ";
        emit_block(*s->body, true);
        out_ << "\n";
        return;
    }

    if (const auto* s = dynamic_cast<const ForStmt*>(&stmt)) {
        int loop_id = loop_counter_++;
        emit_indent();
        out_ << "for (int64_t " << mangle(s->loop_var) << " = ";
        emit_expr(*s->start_expr);
        out_ << ", ex_end_" << loop_id << " = ";
        emit_expr(*s->end_expr);
        out_ << "; " << mangle(s->loop_var) << " < ex_end_" << loop_id
             << "; ++" << mangle(s->loop_var) << ") ";
        emit_block(*s->body, true);
        out_ << "\n";
        return;
    }

    if (const auto* s = dynamic_cast<const ReturnStmt*>(&stmt)) {
        emit_indent();
        if (s->value) {
            out_ << "return ";
            emit_expr(*s->value);
            out_ << ";\n";
        } else {
            out_ << "return;\n";
        }
        return;
    }

    if (const auto* s = dynamic_cast<const PrintStmt*>(&stmt)) {
        emit_indent();
        out_ << "exatron_rt::print_val(";
        emit_expr(*s->expr);
        out_ << ");\n";
        return;
    }

    if (const auto* s = dynamic_cast<const ExprStmt*>(&stmt)) {
        emit_indent();
        emit_expr(*s->expr);
        out_ << ";\n";
        return;
    }
}

// emit_expr

void CodeGenerator::emit_expr(const Expr& expr) {
    if (const auto* e = dynamic_cast<const IntLitExpr*>(&expr)) {
        out_ << "static_cast<int64_t>(" << e->value << ")";
        return;
    }

    if (const auto* e = dynamic_cast<const FloatLitExpr*>(&expr)) {
        std::ostringstream fs;
        fs << std::setprecision(9) << e->value;
        std::string s = fs.str();
        if (s.find('.') == std::string::npos && s.find('e') == std::string::npos &&
            s.find('E') == std::string::npos) {
            s += ".0";
        }
        out_ << s << "f";
        return;
    }

    if (const auto* e = dynamic_cast<const BoolLitExpr*>(&expr)) {
        out_ << (e->value ? "true" : "false");
        return;
    }

    if (const auto* e = dynamic_cast<const StringLitExpr*>(&expr)) {
        out_ << "std::string(\"";
        for (unsigned char c : e->value) {
            if (c == '\\') out_ << "\\\\";
            else if (c == '"') out_ << "\\\"";
            else if (c == '\n') out_ << "\\n";
            else if (c == '\t') out_ << "\\t";
            else if (c >= 32 && c <= 126) out_ << static_cast<char>(c);
            else {
                out_ << '\\'
                     << static_cast<char>('0' + ((c >> 6) & 7))
                     << static_cast<char>('0' + ((c >> 3) & 7))
                     << static_cast<char>('0' + (c & 7));
            }
        }
        out_ << "\")";
        return;
    }

    if (const auto* e = dynamic_cast<const IdentExpr*>(&expr)) {
        out_ << mangle(e->name);
        return;
    }

    if (const auto* e = dynamic_cast<const TensorInitExpr*>(&expr)) {
        out_ << "exatron_rt::Tensor::" << e->kind << "(" << e->rows << ", " << e->cols << ")";
        return;
    }

    if (const auto* e = dynamic_cast<const TensorLitExpr*>(&expr)) {
        int rows = e->inferred_type.shape[0];
        int cols = e->inferred_type.shape[1];
        out_ << "exatron_rt::Tensor(" << rows << ", " << cols << ", {";

        auto* first_inner = dynamic_cast<const TensorLitExpr*>(e->elements[0].get());
        if (first_inner) {
            bool first = true;
            for (const auto& row_expr : e->elements) {
                const auto* row = dynamic_cast<const TensorLitExpr*>(row_expr.get());
                for (const auto& elem : row->elements) {
                    if (!first) out_ << ", ";
                    first = false;
                    out_ << "static_cast<float>(";
                    emit_expr(*elem);
                    out_ << ")";
                }
            }
        } else {
            for (std::size_t i = 0; i < e->elements.size(); ++i) {
                if (i > 0) out_ << ", ";
                out_ << "static_cast<float>(";
                emit_expr(*e->elements[i]);
                out_ << ")";
            }
        }
        out_ << "})";
        return;
    }

    if (const auto* e = dynamic_cast<const UnaryExpr*>(&expr)) {
        if (e->op == TokenType::Minus) out_ << "(-";
        else if (e->op == TokenType::Not) out_ << "(!";
        emit_expr(*e->operand);
        out_ << ")";
        return;
    }

    if (const auto* e = dynamic_cast<const BinaryExpr*>(&expr)) {
        if (e->op == TokenType::MatMul) {
            out_ << "exatron_rt::matmul(";
            emit_expr(*e->lhs);
            out_ << ", ";
            emit_expr(*e->rhs);
            out_ << ")";
            return;
        }
        // Element-wise tensor ops
        if (e->lhs->inferred_type.base == BaseType::Tensor &&
            e->rhs->inferred_type.base == BaseType::Tensor) {
            out_ << "(";
            emit_expr(*e->lhs);
            switch (e->op) {
                case TokenType::Plus:  out_ << " + "; break;
                case TokenType::Minus: out_ << " - "; break;
                case TokenType::Star:  out_ << " * "; break;
                default: break;
            }
            emit_expr(*e->rhs);
            out_ << ")";
            return;
        }
        // Integer division and modulo safety
        if (e->op == TokenType::Slash && e->lhs->inferred_type.base == BaseType::Int) {
            out_ << "exatron_rt::idiv(";
            emit_expr(*e->lhs);
            out_ << ", ";
            emit_expr(*e->rhs);
            out_ << ")";
            return;
        }
        if (e->op == TokenType::Percent && e->lhs->inferred_type.base == BaseType::Int) {
            out_ << "exatron_rt::imod(";
            emit_expr(*e->lhs);
            out_ << ", ";
            emit_expr(*e->rhs);
            out_ << ")";
            return;
        }
        // Scalar binary
        out_ << "(";
        emit_expr(*e->lhs);
        switch (e->op) {
            case TokenType::Plus:    out_ << " + "; break;
            case TokenType::Minus:   out_ << " - "; break;
            case TokenType::Star:    out_ << " * "; break;
            case TokenType::Slash:   out_ << " / "; break;
            case TokenType::Percent: out_ << " % "; break;
            case TokenType::Eq:      out_ << " == "; break;
            case TokenType::Neq:     out_ << " != "; break;
            case TokenType::Lt:      out_ << " < "; break;
            case TokenType::Leq:     out_ << " <= "; break;
            case TokenType::Gt:      out_ << " > "; break;
            case TokenType::Geq:     out_ << " >= "; break;
            case TokenType::And:     out_ << " && "; break;
            case TokenType::Or:      out_ << " || "; break;
            default: break;
        }
        emit_expr(*e->rhs);
        out_ << ")";
        return;
    }

    if (const auto* e = dynamic_cast<const CallExpr*>(&expr)) {
        if (e->callee == "main") {
            out_ << "main(";
        } else {
            out_ << mangle(e->callee) << "(";
        }
        for (std::size_t i = 0; i < e->args.size(); ++i) {
            if (i > 0) out_ << ", ";
            emit_expr(*e->args[i]);
        }
        out_ << ")";
        return;
    }

    if (const auto* e = dynamic_cast<const MemberCallExpr*>(&expr)) {
        emit_expr(*e->object);
        out_ << "." << e->method << "(";
        for (std::size_t i = 0; i < e->args.size(); ++i) {
            if (i > 0) out_ << ", ";
            emit_expr(*e->args[i]);
        }
        out_ << ")";
        return;
    }
}

} // namespace exatron
