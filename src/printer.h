#ifndef YUNOLEX_PRINTER_H
#define YUNOLEX_PRINTER_H

#include <fstream>
#include <map>

#include "automata/automata.h"

namespace yunolex {

class Token;

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

class Printer {
public:
    virtual ~Printer() {
        _outfile.close();
    }

    static Printer* instance(Language lang, std::string output);

    virtual void outputAutomata(std::map<Token*, Automata*>*) = 0;
protected:
    explicit Printer(std::string input, std::string output);

    std::ofstream _outfile;
};

class CppPrinter final : public Printer {
public:
    explicit CppPrinter(std::string output) : Printer("src/lexers/lexcpp.h", output) {} 

    void outputAutomata(std::map<Token*, Automata*>* automata) override;
protected:
    void printSet(std::set<std::string>& set);
};

}

#endif