# Feature 02: Lexical Analyzer (Tokenizer)

## 1. Objective
Implement the complete Exatron lexical analyzer in `include/token.hpp`, `include/lexer.hpp`, and `src/lexer.cpp`. The lexer converts raw `.ex` source code into a flat `std::vector<Token>` stream with exact line and column source locations and clear lexical diagnostics.

## 2. Token Specification (`include/token.hpp`)
Create `include/token.hpp` defining `enum class TokenType`, `struct SourceLoc`, and `struct Token`:

```cpp
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
```

## 3. Lexer Interface (`include/lexer.hpp`)
Create `include/lexer.hpp` with the `Lexer` class declaration:

```cpp
#pragma once
#include "token.hpp"
#include <string>
#include <vector>
#include <stdexcept>

namespace exatron {

class LexerError : public std::runtime_error {
public:
    SourceLoc loc;
    LexerError(const std::string& msg, SourceLoc l)
        : std::runtime_error(msg), loc(l) {}
};

class Lexer {
public:
    explicit Lexer(std::string source);
    std::vector<Token> tokenize();

private:
    std::string src_;
    std::size_t pos_ = 0;
    int line_ = 1;
    int col_ = 1;

    bool is_at_end() const;
    char peek(std::size_t offset = 0) const;
    char advance();
    bool match(char expected);
    void skip_whitespace_and_comments();

    Token read_number(SourceLoc start_loc);
    Token read_identifier_or_keyword(SourceLoc start_loc);
    Token read_string(SourceLoc start_loc);
};

} // namespace exatron
```

## 4. Implementation Requirements (`src/lexer.cpp`)
Implement `src/lexer.cpp` adhering to these rules:

1. **Comments & Whitespace**:
   - Skip spaces (`' '`), tabs (`'\t'`), carriage returns (`'\r'`), and newlines (`'\n'`).
   - Increment `line_` and reset `col_ = 1` on `'\n'`.
   - Support single-line comments starting with `//` until the end of the line.
   - Support multi-line block comments `/* ... */` and throw `LexerError` if unterminated before EOF.

2. **Numbers (`read_number`)**:
   - Scan contiguous digits `[0-9]+`.
   - If followed by a single `.` and another digit `[0-9]` (note: do **not** consume `..` range operator as a decimal point! Check `peek() == '.' && peek(1) != '.'`), consume the `.` and fractional digits `[0-9]+` and emit `TokenType::FloatLit`.
   - Otherwise emit `TokenType::IntLit`.

3. **Identifiers & Keywords (`read_identifier_or_keyword`)**:
   - Start with `[a-zA-Z_]` followed by `[a-zA-Z0-9_]*`.
   - Lookup in a static `std::unordered_map<std::string_view, TokenType>` for all reserved keywords:
     - `"let" -> Let`, `"fn" -> Fn`, `"return" -> Return`, `"if" -> If`, `"else" -> Else`, `"while" -> While`, `"for" -> For`, `"in" -> In`, `"print" -> Print`, `"true" -> True`, `"false" -> False`
     - `"int" -> IntType`, `"float" -> FloatType`, `"bool" -> BoolType`, `"string" -> StringType`, `"void" -> VoidType`, `"tensor" -> TensorType`
     - `"model" -> Model`, `"layer" -> Layer`, `"input" -> Input`, `"Dense" -> Dense`, `"relu" -> Relu`, `"sigmoid" -> Sigmoid`, `"tanh" -> Tanh`, `"softmax" -> Softmax`, `"linear" -> Linear`, `"zeros" -> Zeros`, `"ones" -> Ones`, `"rand" -> Rand`
   - Any non-keyword identifier emits `TokenType::Ident`.

4. **String Literals (`read_string`)**:
   - Enclosed in double quotes `"..."`.
   - Support escape sequences `\n`, `\t`, `\"`, `\\`.
   - Throw `LexerError` with start location if a newline or EOF is reached before closing quote.

5. **Multi-Character Operators**:
   - `->` (`Arrow`), `..` (`DotDot`), `==` (`Eq`), `!=` (`Neq`), `<=` (`Leq`), `>=` (`Geq`), `&&` (`And`), `||` (`Or`).
   - Check two-character sequences before falling back to single-character tokens (`-`, `.`, `=`, `!`, `<`, `>`).
   - `@` maps to `TokenType::MatMul`.

6. **Error Handling**:
   - Any unrecognized character must throw `LexerError` formatted as:
     `"[Lexer Error] Line <line>, Col <col>: Unexpected character '<ch>'"`

## 5. Verification Step
Compile the project with `cmake --build build`. Add a `--dump-tokens <file.ex>` flag capability or temporary test in `main.cpp` to verify that both general-purpose constructs (`for (i in 0..10)`) and neural network constructs (`model Net { layer l1: Dense(4, 8, relu); }`) tokenize cleanly with accurate line and column numbers.
