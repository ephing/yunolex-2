#include <fstream>
#include <iostream>
#include <cstring>

#include "parser/parse.h"
#include "printer.h"

void printUsage() {
    std::cout << "usage: yunolex [-h] [-o FILE] [-l LANG] INPUT" << std::endl;
    std::cout << "  -h, --help  show this help menu and exit" << std::endl;
    std::cout << "  -o FILE     name output file as FILE" << std::endl;
    std::cout << "  -d DIR      output automata as dot files to DIR" << std::endl;
    //std::cout << "  -l LANG     change output language to LANG (supports CPP)" << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string input = "";
    std::string output = "lexer.h";
    std::string dotdir = "";

    // parse arguments
    for ( int i = 1; i < argc; i++ ) {
        if ( !strcmp(argv[i], "-h") || !strcmp(argv[1], "--help") ) {
            printUsage();
            return 0;
        } else if ( !strcmp(argv[i], "-o") ) {
            i++;
            if ( i == argc ) {
                printUsage();
                return 1;
            }
            output = argv[i];
        } else if ( !strcmp(argv[i], "-d") ) {
            i++;
            if ( i == argc ) {
                printUsage();
                return 1;
            }
            dotdir = std::string(argv[i]);
        } else if ( input == "" ) {
            input = argv[i];
        }
    }
    yunolex::info(std::cout, "Finished parsing arguments");
    yunolex::info(std::cout, "Language: CPP");
    yunolex::info(std::cout, "Output file: " + output);

    // Parse input file
    // tokeninfo maps scopes to the token specs within that scope
    std::vector<yunolex::Token*>* tokeninfo;

    try {
        tokeninfo = yunolex::parseFile(input);
    } catch (std::runtime_error& re) {
        std::cerr << re.what() << std::endl;
        return 1;
    }
    yunolex::info(std::cout, "Finished parsing input file.\n");

    // create DFAs from regexes
    // automataInfo maps scopes to tokenname/automata pairs
    std::map<yunolex::Token*, yunolex::Automata*> automataInfo;

    for ( auto token : *tokeninfo ) {
        auto automaton = token->Regex->automata();
        automaton->DFAify();
        automaton->minimize();
        automataInfo.insert({token, automaton});
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
        delete v.first;
        delete v.second;
    }
    yunolex::info(std::cout, "Finished creating lexer file.");

}