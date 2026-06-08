#include <sstream>
#include <string>
#include <map>
#include <iostream>
#include "exprtk.hpp"
#include <regex>

std::string normalize_expr(const std::string& input) {
    std::string result = input;
    result = std::regex_replace(result, std::regex(R"(\|\|)"), " or ");
    result = std::regex_replace(result, std::regex(R"(&&)"),   " and ");
    result = std::regex_replace(result, std::regex(R"(!=)"),   "<>");
    result = std::regex_replace(result, std::regex(R"(!([\w]+))"),   R"(not($1))");
    return result;
}

// template <typename T>
bool evaluate(std::string& expr_str, std::map<std::string, std::string>& map) {
    for (auto& [key, value]: map) {
        int pos;
        pos = expr_str.find(key);
        while (pos != std::string::npos) {
            auto ini = expr_str.substr(0, pos);
            auto end = expr_str.substr(pos+2, expr_str.length());
            expr_str = ini;
            expr_str += value;
            expr_str += end;
            pos = expr_str.find(key);
        }
    }
        typedef exprtk::expression<double> expression_t;
        typedef exprtk::parser<double>     parser_t;

        std::string normalized = normalize_expr(expr_str);

        expression_t expression;
        parser_t parser;
        if (!parser.compile(normalized, expression)) {
            std::cerr << parser.error() << "  [normalized: \"" + normalized + "\"]";
            exit(1);
        }
        return expression.value();
    };

int main(int argc, char **argv) {
    std::map<std::string, std::string> m;
    std::string e1 = "v0 == 1";
    std::string e2 = "(v0 == 2 || v1 > 10)";
    std::string e3 = "(v0 == 2 || (v1 > 10 && v2 > 3)) && v3 == 0";
    std::string e4 = "(v0 == 2 || (v1 > 10 && v2 > 3)) && v3 == -15.000000001 && !v4";
    std::string e5 = "(v0 == 2 || (v1 > 10 && v2 > 3)) && v3 == -15.000000001 && v4";
                    // (1 == 2  or  (15.55 > 10  and   -10 > 3))  and   -15.000000001 == -15.000000001  and  true
                    // False
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