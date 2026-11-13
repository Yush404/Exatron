#pragma once
#include <string>
#include <string_view>
#include <ostream>

namespace exatron {

enum class TokenType {
    // End of file
    Eof,

    // Literals & Identifiers
    Ident,
    IntLit,
    FloatLit,
    StringLit,

    // General-Purpose Keywords
    Let,
    Fn,
    Return,
    If,
    Else,
    While,
    For,
    In,
    Print,
    True,
    False,

    // Primitive & Tensor Type Keywords
    IntType,
    FloatType,
    BoolType,
    StringType,
    VoidType,
    TensorType,

    // Neural Network DSL Keywords
    Model,
    Layer,
    Input,
    Dense,
    Relu,
    Sigmoid,
    Tanh,
    Softmax,
    Linear,
    Zeros,
    Ones,
    Rand,

    // Operators
    Plus,        // +
    Minus,       // -
    Star,        // *
    Slash,       // /
    Percent,     // %
    MatMul,      // @
    Assign,      // =
    Eq,          // ==
    Neq,         // !=
    Lt,          // <
    Leq,         // <=
    Gt,          // >
    Geq,         // >=
    And,         // &&
    Or,          // ||
    Not,         // !
    Arrow,       // ->
    DotDot,      // ..
    Dot,         // .

    // Delimiters
    LParen,      // (
    RParen,      // )
    LBrace,      // {
    RBrace,      // }
    LBracket,    // [
    RBracket,    // ]
    Comma,       // ,
    Colon,       // :
    Semicolon    // ;
};

struct SourceLoc {
    int line = 1;
    int col = 1;
};

struct Token {
    TokenType type{};
    std::string lexeme{};
    SourceLoc loc{};
};

std::string_view token_type_to_string(TokenType type);

} // namespace exatron

