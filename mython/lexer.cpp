#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>
#include <iostream>
#include <set>

using namespace std;

namespace parse {

bool operator==(const Token& lhs, const Token& rhs) {
    using namespace token_type;

    if (lhs.index() != rhs.index()) {
        return false;
    }
    if (lhs.Is<Char>()) {
        return lhs.As<Char>().value == rhs.As<Char>().value;
    }
    if (lhs.Is<Number>()) {
        return lhs.As<Number>().value == rhs.As<Number>().value;
    }
    if (lhs.Is<String>()) {
        return lhs.As<String>().value == rhs.As<String>().value;
    }
    if (lhs.Is<Id>()) {
        return lhs.As<Id>().value == rhs.As<Id>().value;
    }
    return true;
}

bool operator!=(const Token& lhs, const Token& rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const Token& rhs) {
    using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

    VALUED_OUTPUT(Number);
    VALUED_OUTPUT(Id);
    VALUED_OUTPUT(String);
    VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

    UNVALUED_OUTPUT(Class);
    UNVALUED_OUTPUT(Return);
    UNVALUED_OUTPUT(If);
    UNVALUED_OUTPUT(Else);
    UNVALUED_OUTPUT(Def);
    UNVALUED_OUTPUT(Newline);
    UNVALUED_OUTPUT(Print);
    UNVALUED_OUTPUT(Indent);
    UNVALUED_OUTPUT(Dedent);
    UNVALUED_OUTPUT(And);
    UNVALUED_OUTPUT(Or);
    UNVALUED_OUTPUT(Not);
    UNVALUED_OUTPUT(Eq);
    UNVALUED_OUTPUT(NotEq);
    UNVALUED_OUTPUT(LessOrEq);
    UNVALUED_OUTPUT(GreaterOrEq);
    UNVALUED_OUTPUT(None);
    UNVALUED_OUTPUT(True);
    UNVALUED_OUTPUT(False);
    UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

    return os << "Unknown token :("sv;
}

Lexer::Lexer(std::istream& input) {
    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    if (it == end) {
        tokens_.emplace_back(token_type::Eof{});
        return;
    }

    const std::set<char> chars = {'+', '-', '*', '/', '=', '>', '<', '!', '(', ')', '.', ':', ','};

    int global_indent = 0;
    int current_indent = 0;
    bool is_new_line = true;
    bool is_start_file = true;

    while (!input.eof()) {

        while (input.peek() == ' ') {

            input.get();
            if (is_new_line) {
                current_indent++;
            }
        }
        if (input.peek() == '\n' && is_new_line) {
            input.get();
            continue;
        }

        if (global_indent > current_indent) {
            while (global_indent != current_indent) {
                global_indent = global_indent - 2;
                tokens_.emplace_back(token_type::Dedent{});
            }
        } else if (global_indent < current_indent) {
            while (global_indent != current_indent) {
                global_indent = global_indent + 2;
                tokens_.emplace_back(token_type::Indent{});
            }
        }
        if (std::isalpha(input.peek()) || input.peek() == '_') {


            std::string tmp_token;

            tmp_token += static_cast<char>(input.get());

            while ((std::isalpha(input.peek()) || input.peek() == '_' || std::isdigit(input.peek())) && !input.eof()) {
                tmp_token += static_cast<char>(input.get());
            }

            if (ParseKeyLexem(tmp_token)) {

                is_new_line = false;
                is_start_file = false;
                continue;
            }
            ParseIdLexem(tmp_token);
            is_new_line = false;
            is_start_file = false;
        } else if (std::isdigit(input.peek())) {
            std::string tmp_token;

            tmp_token += static_cast<char>(input.get());

            while (std::isdigit(input.peek())) {
                tmp_token += static_cast<char>(input.get());
            }

            ParseDigitLexem(tmp_token);
            is_new_line = false;
            is_start_file = false;
        } else if (char c = input.peek(); chars.find(c) != chars.end()) {
            ParseCharLexem(static_cast<char>(input.get()), input);
            is_new_line = false;
            is_start_file = false;
        } else if (input.peek() == '\n') {
            input.get();
            if (is_start_file) {
                continue;
            }
            if (is_new_line) {
                continue;
            }
            tokens_.emplace_back(token_type::Newline{});
            is_new_line = true;
            current_indent = 0;
        } else if (input.peek() == '#') {

            input.get();


            while (input.peek() != '\n' && !input.eof()) {

                input.get();
            }
            is_start_file = false;
        } else if (input.peek() == '\"') {
            input.get();
            std::string str;
            while (input.peek() != '\"') {
                if (input.peek() == '\\') {
                    input.get();
                    if (input.peek() == 't') {
                        input.get();
                        str += '\t';
                    } else if (input.peek() == 'n') {
                        input.get();
                        str += '\n';
                    } else if (input.peek() == '\\') {
                        input.get();
                        str += '\\';
                    } else if (input.peek() == 'r') {
                        input.get();
                        str += '\r';
                    } else if (input.peek() == '\"') {
                        input.get();
                        str += '\"';
                    } else if (input.peek() == '\'') {
                        input.get();
                        str += '\'';
                    }
                } else {
                    str += input.get();
                }
            }
            input.get();
            tokens_.emplace_back(token_type::String{str});
            is_new_line = false;
            is_start_file = false;
        } else if (input.peek() == '\'') {
            input.get();
            std::string str;
            while (input.peek() != '\'') {
                if (input.peek() == '\\') {
                    input.get();
                    if (input.peek() == 't') {
                        input.get();
                        str += '\t';
                    } else if (input.peek() == 'n') {
                        input.get();
                        str += '\n';
                    } else if (input.peek() == '\\') {
                        input.get();
                        str += '\\';
                    } else if (input.peek() == 'r') {
                        input.get();
                        str += '\r';
                    } else if (input.peek() == '\"') {
                        input.get();
                        str += '\"';
                    } else if (input.peek() == '\'') {
                        input.get();
                        str += '\'';
                    }
                } else {
                    str += input.get();
                }
            }
            input.get();
            tokens_.emplace_back(token_type::String{str});
            is_new_line = false;
            is_start_file = false;
        }

    }



    if (tokens_.size() != 0) {
        if (tokens_.back() != token_type::Newline{} && tokens_.back() != token_type::Dedent{} && tokens_.back() != token_type::Indent{}) {
            tokens_.emplace_back(token_type::Newline{});
        }
    }




    tokens_.emplace_back(token_type::Eof{});

    /*for (auto& token : tokens_) {
        cout << token << endl;
    }*/

    max_token_ = tokens_.size() - 1;

}

const Token& Lexer::CurrentToken() const {
    return tokens_[current_token_];
}

const Token& Lexer::NextToken() {
    if (current_token_ < max_token_) {
        return tokens_[++current_token_];
    } else {
        return tokens_[max_token_];
    }
}

bool Lexer::ParseKeyLexem(const std::string& lexem) {
    if (lexem == "class") {
        tokens_.emplace_back(token_type::Class{});
    } else if (lexem == "return") {
        tokens_.emplace_back(token_type::Return{});
    } else if (lexem == "if") {
        tokens_.emplace_back(token_type::If{});
    } else if (lexem == "else") {
        tokens_.emplace_back(token_type::Else{});
    } else if (lexem == "def") {
        tokens_.emplace_back(token_type::Def{});
    } else if (lexem == "print") {
        tokens_.emplace_back(token_type::Print{});
    } else if (lexem == "and") {
        tokens_.emplace_back(token_type::And{});
    } else if (lexem == "or") {
        tokens_.emplace_back(token_type::Or{});
    } else if (lexem == "not") {
        tokens_.emplace_back(token_type::Not{});
    } else if (lexem == "==") {
        tokens_.emplace_back(token_type::Eq{});
    } else if (lexem == "!=") {
        tokens_.emplace_back(token_type::NotEq{});
    } else if (lexem == "<=") {
        tokens_.emplace_back(token_type::LessOrEq{});
    } else if (lexem == ">=") {
        tokens_.emplace_back(token_type::GreaterOrEq{});
    } else if (lexem == "None") {
        tokens_.emplace_back(token_type::None{});
    } else if (lexem == "True") {
        tokens_.emplace_back(token_type::True{});
    } else if (lexem == "False") {

        tokens_.emplace_back(token_type::False{});

    } else {
        return false;
    }

    return true;
}

void Lexer::ParseIdLexem(const std::string& lexem) {
    tokens_.emplace_back(token_type::Id{lexem});
}

void Lexer::ParseDigitLexem(const std::string& lexem) {
    int tmp_int = std::stoi(lexem);
    tokens_.emplace_back(token_type::Number{tmp_int});
}

void Lexer::ParseCharLexem(char lexem, istream& input) {
    bool flag = true;
    if (lexem == '!') {
        if (input.peek() == '=') {
            tokens_.emplace_back(token_type::NotEq{});
            input.get();
            flag = false;
        }
    } else if (lexem == '<') {
        if (input.peek() == '=') {
            tokens_.emplace_back(token_type::LessOrEq{});
            input.get();
            flag = false;
        }
    } else if (lexem == '>') {
        if (input.peek() == '=') {
            tokens_.emplace_back(token_type::GreaterOrEq{});
            input.get();
            flag = false;
        }
    } else if (lexem == '=') {
        if (input.peek() == '=') {
            tokens_.emplace_back(token_type::Eq{});
            input.get();
            flag = false;
        }
    }

    if (flag) {
        tokens_.emplace_back(token_type::Char{lexem});
    }
}

}  // namespace parse
