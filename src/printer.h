#ifndef YUNOLEX_PRINTER_H
#define YUNOLEX_PRINTER_H

#include <fstream>
#include <map>
#include <iostream>

#include "automata/automata.h"

namespace yunolex {

enum class Language {
    CPP
};

class PrinterException : public std::exception {
public:
    PrinterException(std::string message) : _message(message) {}

    const char* what() const noexcept override { return _message.c_str(); }
private:
    std::string _message;
};

struct AutomataInfo final : public interfaces::Reference, public interfaces::Stringable {
    explicit AutomataInfo(std::string t, Automata* a, std::pair<std::size_t, std::string> ac) 
        : tokenName(std::move(t)), automaton(useref(a)), action(std::move(ac)) {}
    ~AutomataInfo() { freeref(automaton); }
    std::string tokenName;
    Automata* automaton;
    std::pair<std::size_t, std::string> action;
    std::string toString() const override {
        return "AI[" + tokenName + "," + std::to_string((std::size_t)automaton) + ",{" + std::to_string(action.first) + ",\"" + action.second + "\"}]";
    }
};

class Printer {
public:
    virtual ~Printer() {
        _outfile.close();
    }

    static Printer* instance(Language lang, std::string output);

    virtual void outputAutomata(std::map<std::string, std::vector<AutomataInfo*>>*) = 0;
protected:
    explicit Printer(std::string input, std::string output);
    virtual void end() = 0;

    std::ofstream _outfile;
};

class CppPrinter final : public Printer {
public:
    explicit CppPrinter(std::string output) : Printer("src/lexers/lexcpp.h", output) {} 

    void outputAutomata(std::map<std::string, std::vector<AutomataInfo*>>* automata) override;
protected:
    void end() override {
        _outfile << "\t}) {}\n};\n\n}\n\n#endif\n";
    }
};

}

#endif