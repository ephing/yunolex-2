#ifndef YUNOLEX_LEX_H
#define YUNOLEX_LEX_H

#include <map>
#include <string>
#include <vector>
#include <set>
#include <istream>
#include <algorithm>

#define OUTERSCOPE "$"

namespace Lexer {

struct Automaton {
    Automaton(std::string token, std::string start, std::map<std::string, std::map<char, std::string>> transitions, 
        std::set<std::string> fins, std::set<std::string> in, std::set<std::string> enter, std::set<std::string> leave, bool skip, bool error, std::string errormsg) : 
        _token(std::move(token)), _start(std::move(start)), _current(_start),
        _transitions(std::move(transitions)), _finalStates(std::move(fins)), _in(std::move(in)), 
        _enter(std::move(enter)), _leave(std::move(leave)), _dead(false), _skip(skip), _error(error), _errorMsg(errormsg) {}
    std::string _token, _start, _current;
    std::map<std::string, std::map<char, std::string>> _transitions;
    const std::set<std::string> _finalStates;
    const std::set<std::string> _in;
    const std::set<std::string> _enter;
    const std::set<std::string> _leave;
    bool _dead;
    const bool _skip, _error;
    const std::string _errorMsg;
};

struct Position final {
    Position(std::size_t sLine, std::size_t eLine, std::size_t sCol, std::size_t eCol) : SLine(sLine), ELine(eLine), SCol(sCol), ECol(eCol) {}
    std::size_t SLine, ELine, SCol, ECol;
    [[nodiscard]] std::string toString() const {
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
    [[nodiscard]] const char* what() const noexcept override { return _what.c_str(); }
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
    ILexer(std::vector<Automaton*> automata) : _scope(std::set<std::string>()), _automata(std::move(automata)), _bestFit({ 0, { nullptr, nullptr } }), _position(Position(1,1,0,0)), _text(""), _index(0) {
        _scope.insert(OUTERSCOPE);
    }

    [[nodiscard]] bool readCharacter(char c) {
        std::size_t dead = 0, inscope = 0;
        bool ret = false;
        _text += c;
        for ( auto i = _automata.rbegin(); i != _automata.rend(); i++ ) {
            auto a = *i;
            
            std::set<std::string> intersection;
            std::set_intersection(a->_in.begin(), a->_in.end(), _scope.begin(), _scope.end(), std::inserter(intersection, intersection.begin()));

            if ( intersection.size() == 0 ) continue; // skip machines that aren't in a current scope
            inscope++;

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
        if ( dead == inscope ) {
            if ( _bestFit.second.first == nullptr ) {
                // TODO: experiment with some kind of recovery
                throw LexError(_text, &_position);
            } else {
                ret = true;
                if (!_bestFit.second.second->_skip) _tokenStream.push_back(_bestFit.second.first);
                if (_bestFit.second.second->_error) throw LexError(_bestFit.second.second->_errorMsg, &_position);
                _index = _bestFit.first;
                _position = Position(
                    _bestFit.second.first->Pos.ELine, 
                    _bestFit.second.first->Pos.ELine, 
                    _bestFit.second.first->Pos.ECol,
                    _bestFit.second.first->Pos.ECol
                );
                _text = "";
                for ( auto e : _bestFit.second.second->_enter ) {
                    if ( !_scope.count(e) ) _scope.insert(e);
                }
                for ( auto e : _bestFit.second.second->_leave ) {
                    auto f = _scope.find(e);
                    if ( f != _scope.end() ) _scope.erase(f);
                }
            }
            reset();
        }
        return ret;
    }

    void reset() {
        for ( auto a: _automata ) {
            a->_current = a->_start;
            a->_dead = false;
        }
        _bestFit = { _index, { nullptr, nullptr } };
    }

    std::set<std::string> _scope;
    std::vector<Automaton*> _automata;
    std::pair<std::size_t, std::pair<Token*, Automaton*>> _bestFit;
    std::vector<Token*> _tokenStream;

    Position _position;
    std::string _text;
    std::size_t _index;
};

class Lexer final : public ILexer {
public:
    [[nodiscard]] static std::vector<Token*> lex(std::istream& file) {
        Lexer lex;

        while ( file.peek() != -1 ) {
            char c = file.get();
            if ( c == '\n' ) {
                lex._position.ELine++;
                lex._position.ECol = 0;
            } else {
                lex._position.ECol++;
            }
            
            if ( lex.readCharacter(c) ) {
                file.seekg(lex._index + 1);
            }
            lex._index++;
        }

        if ( lex._bestFit.second.first == nullptr ) {
            throw LexError(lex._text, &lex._position);
        } else {
            if (!lex._bestFit.second.second->_skip) lex._tokenStream.push_back(lex._bestFit.second.first);
            if (lex._bestFit.second.second->_error) throw LexError(lex._bestFit.second.second->_errorMsg, &lex._position);
        }

        return lex._tokenStream;
    }
private:
    Lexer() : ILexer(std::vector<Automaton*>({