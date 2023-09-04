#include "printer.h"
#include "framework/dbg.h"

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

void CppPrinter::outputAutomata(std::map<std::string, std::vector<AutomataInfo*>>* automata) {
    for ( auto scope : *automata ) {
        _outfile << "\t\t{ \"" << scope.first << "\",\n\t\t\tstd::vector<Automaton*>({\n" << std::endl;
        for ( auto a : scope.second ) {
            _outfile << "\t\t\t\tnew Automaton(" << std::endl;
            // token name
            _outfile << "\t\t\t\t\t\"" << a->tokenName << "\"," << std::endl;
            // start state
            _outfile << "\t\t\t\t\t\"" << a->automaton->startState()->toString() << "\"," << std::endl;
            // transition table
            _outfile << "\t\t\t\t\t{";
            bool b = false;
            for ( auto s : *a->automaton->states() ) {
                if ( s->outbound().empty() ) continue;
                if ( b ) _outfile << " ";
                _outfile << "{\"" << s->toString() << "\",{";
                for ( auto t : s->outbound() ) {
                    _outfile << "{\'" << t->symbol()[0] << "\',\"" << t->dest()->toString() << "\"},";
                }
                _outfile << "}},\n\t\t\t\t\t";
                b = true;
            }
            _outfile << "}," << std::endl;
            // final states
            _outfile << "\t\t\t\t\t{";
            for ( auto f : a->automaton->finstates() ) {
                _outfile << "\"" << f->toString() << "\",";
            }
            _outfile << "}," << std::endl;
            // action
            _outfile << "\t\t\t\t\t{ " << a->action.first << ", \"" << a->action.second << "\" }\n\t\t\t\t),\n";          
        }
        _outfile << "\t\t\t})\n\t\t}," << std::endl;
    }
    end();
}

}