#include <fstream>
#include <iostream>
#include <cstring>

#include "parser/parse.h"
#include "parser/regexparser.h"
#include "printer.h"

void printUsage() {
    std::cerr << "yunolex <OPTIONS> <filename>" << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string input = "";
    std::string output = "lexer.h";

    // parse arguments
    for ( int i = 1; i < argc; i++ ) {
        if ( !strcmp(argv[i], "-h") || !strcmp(argv[1], "--help") ) {
            printUsage();
            return 0;
        } else if ( input == "" ) {
            input = argv[i];
        }
    }
    yunolex::info(std::cout, "Finished parsing arguments");
    yunolex::info(std::cout, "Language: CPP");
    yunolex::info(std::cout, "Output file: " + output);

    // Parse input file
    // tokeninfo maps scopes to the token specs within that scope
    std::map<std::string, std::vector<yunolex::Token*>>* tokeninfo;

    try {
        tokeninfo = yunolex::Parser::parseFile(input);
    } catch (std::runtime_error& re) {
        std::cerr << re.what() << std::endl;
        return 1;
    }
    yunolex::info(std::cout, "Finished parsing input file.\n");

    // create DFAs from regexes
    // automataInfo maps scopes to tokenname/automata pairs
    std::map<std::string, std::vector<yunolex::AutomataInfo*>> automataInfo;

    for ( auto scope : *tokeninfo ) {
        automataInfo.insert({ scope.first, std::vector<yunolex::AutomataInfo*>() });
        for ( auto token : scope.second ) {
            auto automaton = token->regex->automata();
            automaton->DFAify();
            automataInfo.at(scope.first).push_back(useref(new yunolex::AutomataInfo(token->tokenName, automaton, token->action)));
        }
    }
    
    for ( auto v : *tokeninfo) {
        interfaces::apply<yunolex::Token*>(v.second, [](yunolex::Token* v) -> void {
            delete v;
        });
    }
    delete tokeninfo;
    yunolex::info(std::cout, "Finished creating automata.\n");

    // Creating lexer file for appropriate language and serializing automata
    try {
        auto p = yunolex::Printer::instance(yunolex::Language::CPP, output);
        p->outputAutomata(&automataInfo);
        delete p;
    } catch (yunolex::PrinterException& p) {
        std::cerr << p.what() << std::endl;
        return 1;
    }
    for ( auto v : automataInfo ) {
        interfaces::apply<yunolex::AutomataInfo*>(v.second, [](yunolex::AutomataInfo* ai) -> void {
            delete ai;
        });
    }
    yunolex::info(std::cout, "Finished creating lexer file.");

}