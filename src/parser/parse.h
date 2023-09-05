#ifndef YUNOLEX_PARSE_H
#define YUNOLEX_PARSE_H

#include <fstream>
#include "regexparser.h"

#define OUTERSCOPE "$"

namespace yunolex {

struct Token final {
    Token() = default;
    ~Token() { freeref(Regex); }
    std::string Name;
    Node* Regex = nullptr;
    std::set<std::string> In;
    std::set<std::string> Enter;
    std::set<std::string> Leave;
    bool Skip = false, Error = false;
    std::string ErrorMsg;
    friend std::ostream& operator<<(std::ostream& out, Token& t) {
        out << t.Name << "{" << std::endl << "\tregex: ";
        if ( t.Regex == nullptr ) out << "nullptr";
        else out << t.Regex->toString();
        out << std::endl << "\tin: { ";
        for ( auto i : t.In) out << i << ", ";
        out << "}" << std::endl;
        out << std::endl << "\tenter: { ";
        for ( auto i : t.Enter) out << i << ", ";
        out << "}" << std::endl;
        out << std::endl << "\tleave: { ";
        for ( auto i : t.Leave) out << i << ", ";
        return out << "}" << std::endl << "}";
    }
};

std::vector<Token*>* parseFile(std::string);
namespace parsehelp {
void parseSet(std::string, std::set<std::string>&);
bool verifyToken(Token*);
inline void trim(std::string&);
}

}

#endif