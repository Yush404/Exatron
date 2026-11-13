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

