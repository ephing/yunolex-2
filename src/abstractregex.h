#ifndef YUNOLEX_ABSTRACTREGEX_H
#define YUNOLEX_ABSTRACTREGEX_H

#include "automata/automata.h"
#include "framework/interfaces.h"
#include <iostream>

namespace yunolex {

class Parser;
class RegexParser;

class Node : public interfaces::Stringable, public interfaces::Reference {
public:
    virtual Automata* automata() const = 0;
    virtual std::string name() const = 0;
protected:
    static bool shouldNest(Node* n) {
        std::string name = n->name();
        return !(name == "Star" || name == "Plus" || name == "Question" || name == "Symbol");
    }
};

class BinaryNode : public Node {
public:
    BinaryNode(Node* left, Node* right) : _left(useref(left)), _right(useref(right)) {}
    ~BinaryNode() {
        freeref(_left);
        freeref(_right);
    }

    Node* left() const { return _left; }
    Node* right() const { return _right; }
protected:
    Node* _left;
    Node* _right;
};

class UnaryNode : public Node {
public:
    UnaryNode(Node* body) : _body(useref(body)) {}
    ~UnaryNode() { freeref(_body); }

    Node* body() const { return _body; }
protected:
    Node* _body;
};

class Concatenation : public BinaryNode {
public:
    Concatenation(Node* left, Node* right) : BinaryNode(left, right) {}

    Automata* automata() const override {
        auto left = _left->automata();
        auto right = _right->automata();
        left->concatenateSubsume(right);
        delete right;
        return left;
    }

    std::string name() const override {
        return "Concatentation";
    }

    std::string toString() const override {
        return _left->toString() + _right->toString();
    }
};

class Alternation : public BinaryNode {
public:
    Alternation(Node* left, Node* right) : BinaryNode(left, right) {}

    Automata* automata() const override {
        auto n = new Automata(new State(false));
        auto left = _left->automata();
        auto right = _right->automata();
        n->startState()->addEdge(left->startState(), EPS);
        n->startState()->addEdge(right->startState(), EPS);
        n->assumeStates(left->states());
        n->assumeStates(right->states());
        delete left;
        delete right;
        return n;
    }

    std::string name() const override {
        return "Alternation";
    }

    std::string toString() const override {
        return "(" + _left->toString() + "|" + _right->toString() + ")";
    }
};

class Star : public UnaryNode {
public:
    Star(Node* body) : UnaryNode(body) {}

    Automata* automata() const override {
        auto n = new Automata(new State(true));
        auto body = _body->automata();
        n->startState()->addEdge(body->startState(), EPS);
        interfaces::apply<IState*>(body->finstates(), [n](IState* state) -> void {
            state->addEdge(n->startState(), EPS);
        });
        body->clearFinal();
        n->assumeStates(body->states());
        delete body;
        return n;
    }

    std::string toString() const override {
        if ( Node::shouldNest(_body) ) {
            return "(" + _body->toString() + ")*";
        }
        return _body->toString() + "*";
    }

    std::string name() const override {
        return "Star";
    }
};

class Plus final : public Concatenation {
public:
    Plus(Node* body) : Concatenation(body, new Star(body)) {}

    std::string name() const override {
        return "Plus";
    }

    std::string toString() const override {
        if ( Node::shouldNest(_left) ) {
            return "(" + _left->toString() + ")+";
        }
        return _left->toString() + "+";
    }
};

class Question final : public UnaryNode {
public:
    Question(Node* body) : UnaryNode(body) {}

    Automata* automata() const override {
        auto n = new Automata(new State(false));
        auto end = new State(true);
        n->assumeState(end);
        n->startState()->addEdge(end, EPS);
        auto body = _body->automata();
        n->startState()->addEdge(body->startState(), EPS);
        interfaces::apply<IState*>(body->finstates(), [end](IState* state) -> void {
            state->addEdge(end, EPS);
        });
        body->clearFinal();
        n->assumeStates(body->states());
        delete body;
        return n;
    }

    std::string toString() const override {
        if ( Node::shouldNest(_body) ) {
            return "(" + _body->toString() + ")?";
        }
        return _body->toString() + "?";
    }

    std::string name() const override {
        return "Question";
    }
};

class Interval final : public UnaryNode {
public:
    Interval(Node* body, int lower, int upper) : UnaryNode(body), _lower(lower), _upper(upper) {}

    Automata* automata() const override {
        Automata* n = new Automata(new State(true));
        if ( _upper == 0 ) return n;
        for ( int i = 0; i < _lower; i++ ) {
            n->concatenateSubsume(_body->automata());
        }
        if ( _lower == _upper ) return n;
        if ( _upper == -1 ) { // infinite upper bound
            auto n2 = new Automata(new State(true));
            auto body = _body->automata();
            n2->startState()->addEdge(body->startState(), EPS);
            interfaces::apply<IState*>(body->finstates(), [n2](IState* state) -> void {
                state->addEdge(n2->startState(), EPS);
            });
            body->clearFinal();
            n2->assumeStates(body->states());
            delete body;
            n->concatenateSubsume(n2);
            delete n2;
            return n;
        }
        auto lmfin = n->finstates();
        for ( int i = 0; i < _upper - _lower; i++ ) {
            auto temp = _body->automata();
            interfaces::apply<IState*>(lmfin, [temp](IState* state) -> void {
                state->addEdge(temp->startState(), EPS);
            });
            lmfin = temp->finstates();
            n->assumeStates(temp->states());
            delete temp;
        }
        return n;
    }

    std::string toString() const override {
        std::string interval = "{" + std::to_string(_lower);
        if ( _upper == -1 ) interval += ",}";
        else if ( _upper == _lower ) interval += "}";
        else interval += "," + std::to_string(_upper) + "}";
        if ( Node::shouldNest(_body) ) {
            return "(" + _body->toString() + ")" + interval;
        }
        return _body->toString() + interval;
    }

    std::string name() const override {
        return "Interval";
    }

    int lower() const {
        return _lower;
    }

    int upper() const {
        return _upper;
    }
private:
    int _lower;
    int _upper;

    friend Parser;
};

class Symbol final : public Node {
public:
    Symbol(char c) : _symbol(c) {}
    ~Symbol() = default;

    Automata* automata() const override {
        auto n = new Automata(new State(false));
        State* end = new State(true);
        n->assumeState(end);
        std::string s(1, _symbol);
        n->startState()->addEdge(end, s);
        return n;
    }

    std::string name() const override {
        return "Symbol";
    }

    std::string toString() const override {
        std::string s(1, _symbol);
        return s;
    }
protected:
    char _symbol;
};

class CharacterSelect : public Node {
public:
    explicit CharacterSelect(std::set<char> options) : _options(std::move(options)) {}
    explicit CharacterSelect(char low, char high) {
        for ( char i = low; i < high + 1; i++ ) {
            _options.insert(i);
        }
    }

    Automata* automata() const override {
        auto n = new Automata(new State(false));
        auto end = new State(true);
        n->assumeState(end);
        for ( auto s : _options ) {
            std::string ss(1, s);
            n->startState()->addEdge(end, ss);
        }
        return n;
    }

    void merge(CharacterSelect* other) {
        _options.merge(other->_options);
    }

    std::string name() const override { 
        return "CharacterSelect";
    }

    std::string toString() const override {
        std::string op = "";
        for ( auto s : _options ) op += s;
        return "[" + op + "]";
    }
protected:
    std::set<char> _options;

    friend RegexParser;
};

class Wildcard final : public CharacterSelect {
public:
    explicit Wildcard() : CharacterSelect(32, 126) {
        _options.insert('\t');
    }

    std::string name() const override {
        return "Wildcard";
    }

    std::string toString() const override {
        return ".";
    }
};

}

#endif