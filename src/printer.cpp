#include "printer.h"
#include "framework/dbg.h"
#include "parser/parse.h"

#include <filesystem>

namespace yunolex {

Printer::Printer(std::string input, std::string output) {
    std::ifstream infile;
    auto rp = std::filesystem::canonical("/proc/self/exe").parent_path().append(input);
    infile.open(rp.c_str());

    if ( infile.bad() ) {
        throw PrinterException("Could not open lexer template file: " + input);
    }

    _outfile.open(output);

    if ( _outfile.bad() ) {
        infile.close();
        throw PrinterException("Could not open specified output file: " + output);
    }

    std::string c;
    while ( std::getline(infile, c) ) {
        _outfile << c << std::endl;
    }

    infile.close();
}

Printer* Printer::instance(Language lang, std::string output) {
    if ( lang == Language::CPP ) return new CppPrinter(output);
    throw PrinterException("Somehow you chose a language that doesn't exist");
}

void CppPrinter::outputAutomata(std::map<Token*, Automata*>* automata) {
    for ( auto a : *automata ) {
        _outfile << "\t\tnew Automaton(" << std::endl;
        // token name
        _outfile << "\t\t\t\"" << a.first->Name << "\"," << std::endl;
        // start state
        _outfile << "\t\t\t\"" << a.second->startState()->toString() << "\"," << std::endl;
        // transition table
        _outfile << "\t\t\t{";
        bool b = false;
        for ( auto s : *a.second->states() ) {
            if ( s->outbound().empty() ) continue;
            if ( b ) _outfile << " ";
            _outfile << "{\"" << s->toString() << "\",{";
            for ( auto t : s->outbound() ) {
                _outfile << "{\'" << t->symbol() << "\',\"" << t->dest()->toString() << "\"},";
            }
            _outfile << "}}," << std::endl << "\t\t\t";
            b = true;
        }
        _outfile << "}," << std::endl;
        // final states
        _outfile << "\t\t\t{";
        for ( auto f : a.second->finstates() ) {
            _outfile << "\"" << f->toString() << "\",";
        }
        _outfile << "}," << std::endl;
        // in
        printSet(a.first->In);
        _outfile << "}," << std::endl;
        // enter
        printSet(a.first->Enter);
        _outfile << "}," << std::endl;
        // leave
        printSet(a.first->Leave);
        _outfile << "}," << std::endl; 
        _outfile << "\t\t\t" << (a.first->Skip ? "true, " : "false, ") 
            << (a.first->Error ? "true, \"" + a.first->ErrorMsg + "\"" : "false, \"\"") << std::endl;
        _outfile << "\t\t)," << std::endl;
    }
    _outfile << "\t\t})" << std::endl << "\t) {}" << std::endl << "};"
        << std::endl << std::endl << "}" << std::endl << std::endl << "#endif" << std::endl;
}

void CppPrinter::printSet(std::set<std::string>& set) {
    _outfile << "\t\t\t{";
    for ( auto i : set ) {
        _outfile << "\"" << i << "\",";
    }
}

}