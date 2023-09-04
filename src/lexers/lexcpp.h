#ifndef YUNOLEX_LEX_H
#define YUNOLEX_LEX_H

#include <map>
#include <string>
#include <vector>
#include <set>
#include <stack>
#include <istream>
#include <iostream>

// replace "\x18" when compiling
#define OUTERSCOPE "\x18"

namespace Lexer {

struct Automaton {
    Automaton(std::string token, std::string start, std::map<std::string, std::map<char, std::string>> transitions, 
        std::set<std::string> fins, std::pair<std::size_t, std::string> action) : 
        _token(std::move(token)), _start(std::move(start)), _current(_start), 
        _transitions(std::move(transitions)), _finalStates(std::move(fins)), _action(std::move(action)), _dead(false) {}
    std::string _token, _start, _current;
    std::map<std::string, std::map<char, std::string>> _transitions;
    std::set<std::string> _finalStates;
    std::pair<std::size_t, std::string> _action;
    bool _dead;
};

struct Position final {
    Position(std::size_t sLine, std::size_t eLine, std::size_t sCol, std::size_t eCol) : SLine(sLine), ELine(eLine), SCol(sCol), ECol(eCol) {}
    std::size_t SLine, ELine, SCol, ECol;
    std::string toString() const {
        std::string out;
        out = "(L:" + std::to_string(SLine) + ":" + std::to_string(ELine) + ", C:" + std::to_string(SCol) + ":" + std::to_string(ECol) +  ")";
        return out;
    }
    friend std::ostream& operator<<(std::ostream& out, Position& pos) {
        out << "(L:" << pos.SLine << ":" << pos.ELine << ", C:" << pos.SCol << ":" << pos.ECol <<  ")";
        return out;
    }
    friend std::ostream& operator<<(std::ostream& out, const Position& pos) {
        out << "(L:" << pos.SLine << ":" << pos.ELine << ", C:" << pos.SCol << ":" << pos.ECol <<  ")";
        return out;
    }
};

class LexError final : public std::exception {
public:
    LexError(std::string token, Position* pos) : _what("Invalid token at " + pos->toString() + ": " + token) {}
    const char* what() const noexcept override { return _what.c_str(); }
private:
    std::string _what;
};

struct Token {
    Token(std::string name, std::string lexeme, Position pos) : Name(std::move(name)), Lexeme(std::move(lexeme)), Pos(std::move(pos)) {}
    std::string Name;
    std::string Lexeme;
    Position Pos;
    friend std::ostream& operator<<(std::ostream& out, Token& token) {
        out << "[" << token.Name << ", \"" << token.Lexeme << "\", " << token.Pos << "]";
        return out;
    }
    friend std::ostream& operator<<(std::ostream& out, const Token& token) {
        out << "[" << token.Name << ", \"" << token.Lexeme << "\", " << token.Pos << "]";
        return out;
    }
    std::string toString() const {
        return "[" + Name + ", \"" + Lexeme + "\", " + Pos.toString() + "]";
    }
};

class ILexer {
protected:
    ILexer(std::map<std::string, std::vector<Automaton*>> automata) : _scope(std::stack<std::string>()), _automata(std::move(automata)), _bestFit({ 0, { nullptr, nullptr } }), _position(Position(1,1,0,0)), _text(""), _index(0) {
        _scope.push(OUTERSCOPE);
    }

    void readCharacter(char c) {
        std::size_t dead = 0;
        _text += c;
        for ( auto i = _automata.at(_scope.top()).rbegin(); i != _automata.at(_scope.top()).rend(); i++ ) {
            auto a = *i;
            if ( !a->_dead && a->_transitions.count(a->_current) && a->_transitions.at(a->_current).count(c) ) {
                a->_current = a->_transitions.at(a->_current).at(c);
                if ( a->_finalStates.count(a->_current) ) {
                    delete _bestFit.second.first;
                    _bestFit = { _index , { new Token(a->_token, _text, _position), a } };
                }
            } else {
                a->_dead = true;
                dead++;
            }
        }
        if ( dead == _automata.at(_scope.top()).size() ) {
            if ( _bestFit.second.first == nullptr ) {
                // TODO: experiment with some kind of recovery
                throw LexError(_text, &_position);
            } else {
                _tokenStream.push_back(_bestFit.second.first);
                _index = _bestFit.first;
                _position = Position(
                    _bestFit.second.first->Pos.ELine, 
                    _bestFit.second.first->Pos.ELine, 
                    _bestFit.second.first->Pos.ECol,
                    _bestFit.second.first->Pos.ECol
                );
                _text = "";
                switch ( _bestFit.second.second->_action.first ) {
                case 0:
                    break;
                case 1:
                    _scope.push(_bestFit.second.second->_action.second);
                    break;
                case 2:
                    _scope.pop();
                }
            }
            reset();
        }
    }

    void reset() {
        for ( auto scope : _automata ) {
            for ( auto a : scope.second ) {
                a->_current = a->_start;
                a->_dead = false;
            }
        }
        _bestFit = { _index, { nullptr, nullptr } };
    }

    std::stack<std::string> _scope;
    std::map<std::string, std::vector<Automaton*>> _automata;
    std::pair<std::size_t, std::pair<Token*, Automaton*>> _bestFit;
    std::vector<Token*> _tokenStream;

    Position _position;
    std::string _text;
    std::size_t _index;
};

class Lexer final : public ILexer {
public:
    static std::vector<Token*> lex(std::istream& file) {
        Lexer lex;
        std::size_t ssize = 0;

        while ( file.peek() != -1 ) {
            char c = file.get();
            if ( c == '\n' ) {
                lex._position.ELine++;
                lex._position.ECol = 0;
            } else {
                lex._position.ECol++;
            }
            lex.readCharacter(c);
            if ( lex._tokenStream.size() != ssize ) {
                file.seekg(lex._index + 1);
                ssize = lex._tokenStream.size();
            }
            lex._index++;
        }

        if ( lex._bestFit.second.first == nullptr ) {
            throw LexError(lex._text, &lex._position);
        } else {
            lex._tokenStream.push_back(lex._bestFit.second.first);
        }

        return lex._tokenStream;
    }
private:
    Lexer() : ILexer({
        