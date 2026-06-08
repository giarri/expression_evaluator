#include <sstream>
#include <string>
#include <map>
#include <iostream>
#include <ranges>
#include <regex>

#include <stdexcept>

// ── Tokeniser ──────────────────────────────────────────────────────────────

struct Token {
    enum Type { NUM, BOOL, OP, LPAREN, RPAREN, END } type;
    std::string text;
    double      num  = 0;
    bool        bval = false;
};

class Lexer {
    const std::string& s;
    size_t pos = 0;

    void skip() { while (pos < s.size() && isspace(s[pos])) ++pos; }

public:
    explicit Lexer(const std::string& s) : s(s) {}

    Token next() {
        skip();
        if (pos >= s.size()) return {Token::END};

        // Multi-char operators
        if (pos + 1 < s.size()) {
            std::string two = s.substr(pos, 2);
            if (two == "==" || two == "!=" ||
                two == ">=" || two == "<=" ||
                two == "&&" || two == "||") {
                pos += 2;
                return {Token::OP, two};
            }
        }

        char c = s[pos];

        if (c == '(') { ++pos; return {Token::LPAREN, "("}; }
        if (c == ')') { ++pos; return {Token::RPAREN, ")"}; }
        if (c == '>' || c == '<') { return {Token::OP, std::string(1, s[pos++])}; }
        if (c == '!') { ++pos; return {Token::OP, "!"}; }

        // Number (optional leading minus)
        if (isdigit(c) || (c == '-' && pos+1 < s.size() && isdigit(s[pos+1]))) {
            size_t start = pos;
            if (c == '-') ++pos;
            while (pos < s.size() && (isdigit(s[pos]) || s[pos] == '.')) ++pos;
            std::string num = s.substr(start, pos - start);
            return {Token::NUM, num, std::stod(num)};
        }

        // Keyword: true / false
        if (isalpha(c)) {
            size_t start = pos;
            while (pos < s.size() && isalpha(s[pos])) ++pos;
            std::string word = s.substr(start, pos - start);
            if (word == "true")  return {Token::BOOL, word, 0, true};
            if (word == "false") return {Token::BOOL, word, 0, false};
            throw std::runtime_error("Unknown keyword: " + word);
        }

        throw std::runtime_error(std::string("Unexpected char: ") + c);
    }
};

// ── Recursive-descent parser / evaluator ──────────────────────────────────
//
//  Grammar (lowest → highest precedence):
//    expr   → and  ( "||" and )*
//    and    → cmp  ( "&&" cmp )*
//    cmp    → unary ( ("==" | "!=" | "<" | ">" | "<=" | ">=") unary )?
//    unary  → "!" unary | atom
//    atom   → NUMBER | BOOL | "(" expr ")"

class Parser {
    Lexer lex;
    Token cur;

    void advance() { cur = lex.next(); }

    bool expect(Token::Type t) {
        if (cur.type != t)
            throw std::runtime_error("Parse error near '" + cur.text + "'");
        advance();
        return true;
    }

public:
    explicit Parser(const std::string& s) : lex(s) { advance(); }

    bool parse() {
        bool v = expr();
        if (cur.type != Token::END)
            throw std::runtime_error("Unexpected token: " + cur.text);
        return v;
    }

private:
    bool expr() {                          // ||
        bool v = andExpr();
        while (cur.type == Token::OP && cur.text == "||") {
            advance();
            bool r = andExpr();
            v = v || r;
        }
        return v;
    }

    bool andExpr() {                       // &&
        bool v = cmp();
        while (cur.type == Token::OP && cur.text == "&&") {
            advance();
            bool r = cmp();
            v = v && r;
        }
        return v;
    }

    bool cmp() {                           // ==  !=  <  >  <=  >=
        bool v = unary();
        if (cur.type == Token::OP &&
            (cur.text == "==" || cur.text == "!=" ||
             cur.text == "<"  || cur.text == ">"  ||
             cur.text == "<=" || cur.text == ">=")) {
            std::string op = cur.text;
            advance();
            bool r = unary();
            // For numeric comparisons we need the raw double values;
            // re-evaluate both sides as doubles via numericValue()
            // — but we already consumed them as bools.
            // Solution: promote bool to double (true→1, false→0).
            double ld = v, rd = r;
            if (op == "==") return std::abs(ld - rd) < 1e-15 || ld == rd;
            if (op == "!=") return ld != rd;
            if (op == "<")  return ld <  rd;
            if (op == ">")  return ld >  rd;
            if (op == "<=") return ld <= rd;
            if (op == ">=") return ld >= rd;
        }
        return v;
    }

    bool unary() {                         // !
        if (cur.type == Token::OP && cur.text == "!") {
            advance();
            return !unary();
        }
        return atom();
    }

    bool atom() {
        if (cur.type == Token::BOOL) {
            bool v = cur.bval; advance(); return v;
        }
        if (cur.type == Token::NUM) {
            double v = cur.num; advance(); return v != 0.0;
        }
        if (cur.type == Token::LPAREN) {
            advance();
            bool v = expr();
            expect(Token::RPAREN);
            return v;
        }
        throw std::runtime_error("Expected value, got: " + cur.text);
    }
};

// ── Numeric-aware comparison helper ───────────────────────────────────────
//
// The cmp() above works for bool vs bool, but for expressions like
//   "15.55 > 10" or "-15.000000001 == 0"
// we need to track the *original numeric value*, not just its truthiness.
// The cleanest fix: use a variant-like Value type.

struct Value {
    bool   isBool = false;
    double num    = 0;
    bool   bval   = false;

    bool asBool() const { return isBool ? bval : (num != 0.0); }
    double asNum() const { return isBool ? (bval ? 1.0 : 0.0) : num; }
};

// ── Full implementation with Value ────────────────────────────────────────

class Evaluator {
    Lexer lex;
    Token cur;

    void advance() { cur = lex.next(); }

public:
    explicit Evaluator(const std::string& s) : lex(s) { advance(); }

    bool evaluate() {
        Value v = expr();
        if (cur.type != Token::END)
            throw std::runtime_error("Unexpected token: " + cur.text);
        return v.asBool();
    }

private:
    Value expr() {
        Value v = andExpr();
        while (cur.type == Token::OP && cur.text == "||") {
            advance();
            Value r = andExpr();
            v = Value{true, 0, v.asBool() || r.asBool()};
        }
        return v;
    }

    Value andExpr() {
        Value v = cmp();
        while (cur.type == Token::OP && cur.text == "&&") {
            advance();
            Value r = cmp();
            v = Value{true, 0, v.asBool() && r.asBool()};
        }
        return v;
    }

    Value cmp() {
        Value v = unary();
        if (cur.type == Token::OP &&
            (cur.text == "==" || cur.text == "!=" ||
             cur.text == "<"  || cur.text == ">"  ||
             cur.text == "<=" || cur.text == ">=")) {
            std::string op = cur.text;
            advance();
            Value r = unary();
            double ld = v.asNum(), rd = r.asNum();
            bool res = false;
            if (op == "==") res = (ld == rd);
            else if (op == "!=") res = (ld != rd);
            else if (op == "<")  res = (ld <  rd);
            else if (op == ">")  res = (ld >  rd);
            else if (op == "<=") res = (ld <= rd);
            else if (op == ">=") res = (ld >= rd);
            return Value{true, 0, res};
        }
        return v;
    }

    Value unary() {
        if (cur.type == Token::OP && cur.text == "!") {
            advance();
            Value v = unary();
            return Value{true, 0, !v.asBool()};
        }
        return atom();
    }

    Value atom() {
        if (cur.type == Token::BOOL) {
            Value v{true, 0, cur.bval}; advance(); return v;
        }
        if (cur.type == Token::NUM) {
            Value v{false, cur.num, false}; advance(); return v;
        }
        if (cur.type == Token::LPAREN) {
            advance();
            Value v = expr();
            if (cur.type != Token::RPAREN)
                throw std::runtime_error("Expected ')'");
            advance();
            return v;
        }
        throw std::runtime_error("Expected value, got: '" + cur.text + "'");
    }
};


bool evaluate(std::string& exp_str, std::map<std::string, std::string>& map) {
    for (auto& [fst, snd]: map) {
        while (exp_str.find(fst) != std::string::npos) {
            int pos = exp_str.find(fst);
            std::string ini = exp_str.substr(0, pos);
            std::string end = exp_str.substr(pos+2, exp_str.length());
            exp_str = ini;
            exp_str += snd;
            exp_str += end;
        }
    }
    return Evaluator(exp_str).evaluate();
}


int main(int argc, char **argv) {
    std::map<std::string, std::string> m;
    std::string e1 = "v0 == 1";
    std::string e2 = "(v0 == 2 || v1 > 10)";
    std::string e3 = "(v0 == 2 || (v1 > 10 && v2 > 3)) && v3 == 0";
    std::string e4 = "(v0 == 2 || (v1 > 10 && v2 > 3)) && v3 == -15.000000001 && !v4";
    std::string e5 = "(v0 == 2 || (v1 > 10 && v2 > 3)) && v3 == -15.000000001 && v4";
    std::string e6 = "((v0 == 2 || (v1 > 10 && v2 > 3)) && v3 == -15.000000001 && v4) && (v5 == !v4)";
    std::string e7 = "true";
    m["v0"] = "1";
    m["v1"] = "15.55";
    m["v2"] = " -10";
    m["v3"] = " -15.000000001";
    m["v4"] = "true";
    m["v5"] = "false";
    bool testCorrect;
    testCorrect = evaluate(e1, m) &&
                  evaluate(e2, m) &&
                  !evaluate(e3, m) &&
                  !evaluate(e4, m) &&
                  evaluate(e5, m) &&
                  evaluate(e6, m) &&
                  evaluate(e7, m);
    std::cout << (testCorrect ? "Good job!" : "Uhm, please retry!") << std::endl;
    return 0;
}