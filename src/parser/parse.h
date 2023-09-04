// parses yunolex specification files
#ifndef YUNOLEX_PARSE_H
#define YUNOLEX_PARSE_H

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <iostream>

#include "../framework/interfaces.h"
#include "regexparser.h"

#define OUTERSCOPE "topscope"

namespace yunolex {

struct Token : public interfaces::Stringable {
    Token() = delete;
    Token(std::string rgxstr, Node* rgx, std::string name, std::pair<std::size_t, std::string> act) 
        : regexstr(rgxstr), regex(useref(rgx)), tokenName(name), action(act) {}
    ~Token() { freeref(regex); };
    std::string regexstr;
    Node* regex;
    std::string tokenName;
    std::pair<std::size_t, std::string> action; // 0: no action, 1: enter scope, 2: leave scope

    std::string toString() const override {
        return tokenName + "[" + regexstr + ", {" + std::to_string(action.first) +"," + action.second + "}]";
    }

    friend std::ostream& operator<<(std::ostream& out, const Token& t) {
        return out << t.toString();
    }
};

class Parser {
public:
    static std::map<std::string, std::vector<Token*>>* parseFile(std::string filename) {
        std::ifstream input;
        input.open(filename);
        
        if ( input.bad() ) {
            throw std::runtime_error("Invalid input file: " + filename);
        }

        auto out = new std::map<std::string, std::vector<Token*>>();
        out->insert({ OUTERSCOPE, std::vector<Token*>() });
        info(std::cout, "Parsing: Opened file");

        auto p = Parser(input);
        p.parse(out);
        info(std::cout, "Parsing: Finished parsing");

        return out;
    }
protected:
    Parser(std::ifstream& input) : _input(input), _line(0) {}

    void parse(std::map<std::string, std::vector<Token*>>* out) {
        std::string scope = OUTERSCOPE, current;
        while ( std::getline(_input, current) ) {
            _line++;
            trim(current);
            if ( current == "" ) continue;
            info(std::cout, "Parsing line: " + current);
            if ( current.ends_with(':') && current.find(' ') == std::string::npos ) {
                info(std::cout, current + " identified as a scope!");
                scope = "s_" + current.substr(0, current.size() - 1);
                out->insert({ scope, std::vector<Token*>()});
            } else {
                std::string regex, name;
                std::pair<std::size_t, std::string> action;

                std::size_t regexSpace = current.find(' ');
                if ( regexSpace == std::string::npos ) {
                    std::cerr << "Regular Expression not followed by a token name\n";
                    continue;
                }
                regex = current.substr(0, regexSpace);
                current = current.substr(regexSpace + 1);
                std::size_t s;
                if ( (s = current.find("<")) != std::string::npos ) {
                    info(std::cout, "Action: leave scope");
                    std::string temp = current.substr(0,s);
                    trim(temp);
                    name = temp;
                    action = { 2, "" };
                } else if ( (s = current.find(">")) != std::string::npos ) {
                    std::string temp = current.substr(0,s);
                    trim(temp);
                    name = temp;
                    temp = current.substr(s + 1);
                    trim(temp);
                    action = { 1, temp };
                    info(std::cout, "Action: enter scope " + temp);
                } else {
                    name = current;
                    action = { 0, "" };
                    info(std::cout, "Action: none");
                }

                try {
                    auto n = RegexParser::parse(regex, _line, 0);
                    out->at(scope).push_back(new Token(regex, n, name, action));
                } catch (ParserException& p) {
                    std::cerr << p.what() << std::endl;
                }
            }
        }
    }

    inline void trim(std::string& s) const {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));

        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    std::ifstream& _input;
    int _line;
};

}

#endif