#ifndef IONIA_FRONT_LEXER_H_
#define IONIA_FRONT_LEXER_H_

#include <istream>
#include <string>

class Lexer {
public:
    enum class Token {
        End, Error, Id, Num, Char
    };

    Lexer(std::istream &in) : in_(in), last_char_(' '), error_num_(0) {
        in_ >> std::noskipws;
        NextChar();
    }

    Token NextToken();

    unsigned int error_num() const { return error_num_; }
    const std::string &id_val() const { return id_val_; }
    int num_val() const { return num_val_; }
    char char_val() const { return char_val_; }

private:
    void NextChar() { in_ >> last_char_; }
    bool IsEOL() {
        return in_.eof() || last_char_ == '\r' || last_char_ == '\n';
    }

    Token PrintError(const char *message);
    Token HandleId();
    Token HandleNum();
    Token HandleComment();
    Token HandleEOL();

    std::istream &in_;
    char last_char_;
    unsigned int error_num_;
    std::string id_val_;
    int num_val_;
    char char_val_;
};

#endif // IONIA_FRONT_LEXER_H_
