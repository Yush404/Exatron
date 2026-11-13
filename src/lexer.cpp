#include "lexer.hpp"
#include <unordered_map>
#include <string_view>

namespace exatron {

// ── token_type_to_string ────────────────────────────────────────────────────
std::string_view token_type_to_string(TokenType type) {
    switch (type) {
        case TokenType::Eof:        return "Eof";
        case TokenType::Ident:      return "Ident";
        case TokenType::IntLit:     return "IntLit";
        case TokenType::FloatLit:   return "FloatLit";
        case TokenType::StringLit:  return "StringLit";
        case TokenType::Let:        return "Let";
        case TokenType::Fn:         return "Fn";
        case TokenType::Return:     return "Return";
        case TokenType::If:         return "If";
        case TokenType::Else:       return "Else";
        case TokenType::While:      return "While";
        case TokenType::For:        return "For";
        case TokenType::In:         return "In";
        case TokenType::Print:      return "Print";
        case TokenType::True:       return "True";
        case TokenType::False:      return "False";
        case TokenType::IntType:    return "IntType";
        case TokenType::FloatType:  return "FloatType";
        case TokenType::BoolType:   return "BoolType";
        case TokenType::StringType: return "StringType";
        case TokenType::VoidType:   return "VoidType";
        case TokenType::TensorType: return "TensorType";
        case TokenType::Model:      return "Model";
        case TokenType::Layer:      return "Layer";
        case TokenType::Input:      return "Input";
        case TokenType::Dense:      return "Dense";
        case TokenType::Relu:       return "Relu";
        case TokenType::Sigmoid:    return "Sigmoid";
        case TokenType::Tanh:       return "Tanh";
        case TokenType::Softmax:    return "Softmax";
        case TokenType::Linear:     return "Linear";
        case TokenType::Zeros:      return "Zeros";
        case TokenType::Ones:       return "Ones";
        case TokenType::Rand:       return "Rand";
        case TokenType::Plus:       return "Plus";
        case TokenType::Minus:      return "Minus";
        case TokenType::Star:       return "Star";
        case TokenType::Slash:      return "Slash";
        case TokenType::Percent:    return "Percent";
        case TokenType::MatMul:     return "MatMul";
        case TokenType::Assign:     return "Assign";
        case TokenType::Eq:         return "Eq";
        case TokenType::Neq:        return "Neq";
        case TokenType::Lt:         return "Lt";
        case TokenType::Leq:        return "Leq";
        case TokenType::Gt:         return "Gt";
        case TokenType::Geq:        return "Geq";
        case TokenType::And:        return "And";
        case TokenType::Or:         return "Or";
        case TokenType::Not:        return "Not";
        case TokenType::Arrow:      return "Arrow";
        case TokenType::DotDot:     return "DotDot";
        case TokenType::Dot:        return "Dot";
        case TokenType::LParen:     return "LParen";
        case TokenType::RParen:     return "RParen";
        case TokenType::LBrace:     return "LBrace";
        case TokenType::RBrace:     return "RBrace";
        case TokenType::LBracket:   return "LBracket";
        case TokenType::RBracket:   return "RBracket";
        case TokenType::Comma:      return "Comma";
        case TokenType::Colon:      return "Colon";
        case TokenType::Semicolon:  return "Semicolon";
    }
    __builtin_unreachable();
}

// ── Lexer Implementation ────────────────────────────────────────────────────

Lexer::Lexer(std::string source) : src_(std::move(source)) {}

bool Lexer::is_at_end() const {
    return pos_ >= src_.size();
}

char Lexer::peek(std::size_t offset) const {
    if (pos_ + offset >= src_.size()) return '\0';
    return src_[pos_ + offset];
}

char Lexer::advance() {
    char ch = src_[pos_++];
    if (ch == '\n') {
        ++line_;
        col_ = 1;
    } else {
        ++col_;
    }
    return ch;
}

bool Lexer::match(char expected) {
    if (is_at_end() || src_[pos_] != expected) return false;
    advance();
    return true;
}

void Lexer::skip_whitespace_and_comments() {
    while (!is_at_end()) {
        char ch = peek();
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            advance();
        } else if (ch == '/' && peek(1) == '/') {
            // Single-line comment
            while (!is_at_end() && peek() != '\n') advance();
        } else if (ch == '/' && peek(1) == '*') {
            // Multi-line block comment
            SourceLoc start{line_, col_};
            advance(); // consume '/'
            advance(); // consume '*'
            while (!is_at_end()) {
                if (peek() == '*' && peek(1) == '/') {
                    advance(); // consume '*'
                    advance(); // consume '/'
                    break;
                }
                if (is_at_end()) {
                    throw LexerError("[Lexer Error] Line " + std::to_string(start.line)
                        + ", Col " + std::to_string(start.col) + ": Unterminated block comment", start);
                }
                advance();
            }
            if (is_at_end() && !(pos_ >= 2 && src_[pos_-2] == '*' && src_[pos_-1] == '/')) {
                throw LexerError("[Lexer Error] Line " + std::to_string(start.line)
                    + ", Col " + std::to_string(start.col) + ": Unterminated block comment", start);
            }
        } else {
            break;
        }
    }
}

Token Lexer::read_number(SourceLoc start_loc) {
    std::string lexeme;
    while (!is_at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
        lexeme += advance();
    }
    // Check for float: '.' followed by digit, but NOT '..' (range operator)
    if (peek() == '.' && peek(1) != '.' && std::isdigit(static_cast<unsigned char>(peek(1)))) {
        lexeme += advance(); // consume '.'
        while (!is_at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
            lexeme += advance();
        }
        return Token{TokenType::FloatLit, lexeme, start_loc};
    }
    return Token{TokenType::IntLit, lexeme, start_loc};
}

Token Lexer::read_identifier_or_keyword(SourceLoc start_loc) {
    static const std::unordered_map<std::string_view, TokenType> keywords = {
        {"let", TokenType::Let},
        {"fn", TokenType::Fn},
        {"return", TokenType::Return},
        {"if", TokenType::If},
        {"else", TokenType::Else},
        {"while", TokenType::While},
        {"for", TokenType::For},
        {"in", TokenType::In},
        {"print", TokenType::Print},
        {"true", TokenType::True},
        {"false", TokenType::False},
        {"int", TokenType::IntType},
        {"float", TokenType::FloatType},
        {"bool", TokenType::BoolType},
        {"string", TokenType::StringType},
        {"void", TokenType::VoidType},
        {"tensor", TokenType::TensorType},
        {"model", TokenType::Model},
        {"layer", TokenType::Layer},
        {"input", TokenType::Input},
        {"Dense", TokenType::Dense},
        {"relu", TokenType::Relu},
        {"sigmoid", TokenType::Sigmoid},
        {"tanh", TokenType::Tanh},
        {"softmax", TokenType::Softmax},
        {"linear", TokenType::Linear},
        {"zeros", TokenType::Zeros},
        {"ones", TokenType::Ones},
        {"rand", TokenType::Rand},
    };

    std::string lexeme;
    while (!is_at_end() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
        lexeme += advance();
    }

    auto it = keywords.find(lexeme);
    if (it != keywords.end()) {
        return Token{it->second, lexeme, start_loc};
    }
    return Token{TokenType::Ident, lexeme, start_loc};
}

Token Lexer::read_string(SourceLoc start_loc) {
    advance(); // consume opening '"'
    std::string value;
    while (!is_at_end() && peek() != '"') {
        if (peek() == '\n') {
            throw LexerError("[Lexer Error] Line " + std::to_string(start_loc.line)
                + ", Col " + std::to_string(start_loc.col) + ": Unterminated string literal", start_loc);
        }
        if (peek() == '\\') {
            advance(); // consume backslash
            if (is_at_end()) {
                throw LexerError("[Lexer Error] Line " + std::to_string(start_loc.line)
                    + ", Col " + std::to_string(start_loc.col) + ": Unterminated string literal", start_loc);
            }
            char esc = advance();
            switch (esc) {
                case 'n':  value += '\n'; break;
                case 't':  value += '\t'; break;
                case '"':  value += '"';  break;
                case '\\': value += '\\'; break;
                default:   value += esc;  break;
            }
        } else {
            value += advance();
        }
    }
    if (is_at_end()) {
        throw LexerError("[Lexer Error] Line " + std::to_string(start_loc.line)
            + ", Col " + std::to_string(start_loc.col) + ": Unterminated string literal", start_loc);
    }
    advance(); // consume closing '"'
    return Token{TokenType::StringLit, value, start_loc};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        skip_whitespace_and_comments();
        if (is_at_end()) {
            tokens.push_back(Token{TokenType::Eof, "", {line_, col_}});
            break;
        }

        SourceLoc loc{line_, col_};
        char ch = peek();

        // Numbers
        if (std::isdigit(static_cast<unsigned char>(ch))) {
            tokens.push_back(read_number(loc));
            continue;
        }

        // Identifiers and keywords
        if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
            tokens.push_back(read_identifier_or_keyword(loc));
            continue;
        }

        // String literals
        if (ch == '"') {
            tokens.push_back(read_string(loc));
            continue;
        }

        // Multi-character and single-character operators/delimiters
        advance(); // consume the character
        switch (ch) {
            case '+': tokens.push_back({TokenType::Plus, "+", loc}); break;
            case '*': tokens.push_back({TokenType::Star, "*", loc}); break;
            case '%': tokens.push_back({TokenType::Percent, "%", loc}); break;
            case '@': tokens.push_back({TokenType::MatMul, "@", loc}); break;
            case '(': tokens.push_back({TokenType::LParen, "(", loc}); break;
            case ')': tokens.push_back({TokenType::RParen, ")", loc}); break;
            case '{': tokens.push_back({TokenType::LBrace, "{", loc}); break;
            case '}': tokens.push_back({TokenType::RBrace, "}", loc}); break;
            case '[': tokens.push_back({TokenType::LBracket, "[", loc}); break;
            case ']': tokens.push_back({TokenType::RBracket, "]", loc}); break;
            case ',': tokens.push_back({TokenType::Comma, ",", loc}); break;
            case ':': tokens.push_back({TokenType::Colon, ":", loc}); break;
            case ';': tokens.push_back({TokenType::Semicolon, ";", loc}); break;

            case '-':
                if (match('>')) tokens.push_back({TokenType::Arrow, "->", loc});
                else tokens.push_back({TokenType::Minus, "-", loc});
                break;
            case '.':
                if (match('.')) tokens.push_back({TokenType::DotDot, "..", loc});
                else tokens.push_back({TokenType::Dot, ".", loc});
                break;
            case '=':
                if (match('=')) tokens.push_back({TokenType::Eq, "==", loc});
                else tokens.push_back({TokenType::Assign, "=", loc});
                break;
            case '!':
                if (match('=')) tokens.push_back({TokenType::Neq, "!=", loc});
                else tokens.push_back({TokenType::Not, "!", loc});
                break;
            case '<':
                if (match('=')) tokens.push_back({TokenType::Leq, "<=", loc});
                else tokens.push_back({TokenType::Lt, "<", loc});
                break;
            case '>':
                if (match('=')) tokens.push_back({TokenType::Geq, ">=", loc});
                else tokens.push_back({TokenType::Gt, ">", loc});
                break;
            case '&':
                if (match('&')) tokens.push_back({TokenType::And, "&&", loc});
                else throw LexerError("[Lexer Error] Line " + std::to_string(loc.line)
                    + ", Col " + std::to_string(loc.col) + ": Unexpected character '&'", loc);
                break;
            case '|':
                if (match('|')) tokens.push_back({TokenType::Or, "||", loc});
                else throw LexerError("[Lexer Error] Line " + std::to_string(loc.line)
                    + ", Col " + std::to_string(loc.col) + ": Unexpected character '|'", loc);
                break;
            case '/':
                // '/' as division (comments already handled in skip_whitespace_and_comments)
                tokens.push_back({TokenType::Slash, "/", loc});
                break;

            default:
                throw LexerError("[Lexer Error] Line " + std::to_string(loc.line)
                    + ", Col " + std::to_string(loc.col) + ": Unexpected character '" + ch + "'", loc);
        }
    }

    return tokens;
}

} // namespace exatron

