#ifndef YUNOLEX_DFA_H
#define YUNOLEX_DFA_H

#include <functional>
#include <ostream>
#include "../framework/interfaces.h"
#include "state.h"

namespace yunolex {

class Automata : public interfaces::Stringable, public interfaces::Reference {
public:
    explicit Automata(IState* start) : _startState(start), _states(new std::set<IState*>()) {
        assumeState(_startState);
    }

    ~Automata() {
        interfaces::apply<IState*>(*_states, [](IState* s) -> void { freeref(s); });
        delete _states;
    }

    void assumeStates(std::set<IState*>* states) {
        _states->merge(*states);
        interfaces::apply<IState*>(*_states, [this](IState* state) -> void {
            if ( state->isFinal() ) this->_finStates.insert(state);
        });
    }

    void assumeState(IState* state) {
        _states->insert(useref(state));
        if ( state->isFinal() ) _finStates.insert(state);
    }

    [[nodiscard]] static IState* containsState(std::set<IState*>& states, IState* state) {
        for (auto s : states) {
            if (*s == *state) return s;
        }
        return nullptr;
    }

    template <typename T>
    [[nodiscard]] std::set<T> map(std::function<T(IState*)> callback) {
        std::set<T> nstates;
        for (auto s : *_states) {
            nstates.insert(callback(s));
        }
        return nstates;
    }

    void clearFinal() {
        interfaces::apply<IState*>(_finStates, [](IState* s) -> void { s->setFinal(false); });
        _finStates.clear();
    }

    [[nodiscard]] IState* startState() const { return _startState; }

    [[nodiscard]] std::set<IState*>* states() const { return _states; }

    [[nodiscard]] std::set<IState*> finstates() const { return _finStates; }

    // creates dot file graphviz output
    void dot(std::ostream& out) const;

    void removeEpsilonTransitions();

    // Banishes nondeterminism
    void DFAify();

    // concatenates automata (invalidates input automata)
    void concatenateSubsume(Automata*);

    std::string toString() const override {
        return "Automata<states: " + std::to_string(_states->size()) + ">";
    }

protected:
    IState* _startState;
    std::set<IState*>* _states; // maybe create reduceToStateSet??? useful for combining automata?
    std::set<IState*> _finStates;
private:
    void __dfaHelp(StateSet*, std::set<IState*>*);
    void __removeUnreachable();
    void __rUHelp(IState*, std::set<IState*>&);
    //void __removeDead(); // This should do nothing because I *believe* its impossible to have dead states
};

}

#endif