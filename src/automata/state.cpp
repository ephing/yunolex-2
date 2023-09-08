#include "state.h"

namespace yunolex {

IState::~IState() {
    for (auto i : _outbound) freeref(i);
}

void IState::addEdge(IState* dest, std::string sym) {
    Transition* t;
    if ( containsEdge(dest, sym) ) return;
    if (sym == EPS) t = new EpsilonTransition(this, dest);
    else t = new Transition(sym, this, dest);
    _outbound.insert(useref(t));
}

void IState::removeEdge(Transition* t) {
    if ( _outbound.contains(t) ) {
        _outbound.erase(t);
        freeref(t);
    }
}

bool IState::containsEdge(IState* dest, std::string sym) const {
    for ( auto t : _outbound ) {
        if ( *t->dest() == *dest && t->symbol() == sym ) return true;
    }
    return false;
}

IState* IState::nextState(std::string input) const {
    for ( auto t : _outbound ) {
        if ( t->symbol() == input ) return t->dest();
    }
    return nullptr;
}

bool IState::semanticallyEquivalent(IState* other) const {
    if ( other == nullptr || _final != other->_final ) return false;
    if ( _id == other->_id ) return true;
    for ( auto t : _outbound ) {
        auto os = other->nextState(t->symbol());
        if ( os == this && t->dest() == other ) continue; // this is loop
        if ( os == other && t->dest() == this ) continue; // this is also loop
        if ( !t->dest()->semanticallyEquivalent(os) ) return false;
    }
    for ( auto t : other->_outbound ) {
        auto s = nextState(t->symbol());
        if ( s == this && t->dest() == other ) continue;
        if ( s == other && t->dest() == this ) continue;
        if ( !t->dest()->semanticallyEquivalent(s) ) return false;
    }
    return true;
}

std::set<const IState*> IState::transitiveReflexiveClosure(bool epsilons) const {
    std::set<const IState*> visited;
    __trClosure(visited, epsilons);
    return visited;
}

void IState::__trClosure(std::set<const IState*>& visited, bool epsilons) const {
    visited.insert(this);

    for ( auto t : outbound() ) {
        if ( !visited.contains(t->dest()) && (t->getType() == Transition::Type::EPSILON || !epsilons) ) {
            t->dest()->__trClosure(visited, epsilons);
        }
    }
}

std::size_t State::Id = 0;

StateSet::StateSet(std::vector<IState*> states) : IState("", false, IState::Type::SET) {
    // sort states, such that generating multiple state sets with the same states results in the same id
    _states = StateSet::__flatten(states);
    sort(_states.begin(), _states.end(), StateSet::__compareStates);
    std::string id = "S_";
    for (auto s : _states) {
        auto sstr = s->toString();
        id += sstr.substr(2, sstr.size() - 3);
    }
    _id = id + "_";

    for ( auto s : states ) {
        if ( s->isFinal() ) {
            _final = true;
            break;
        }
    }
}

StateSet::~StateSet() {
    interfaces::apply<IState*>(_states, [](IState* state) -> void { freeref(state); });
}

std::vector<IState*> StateSet::__flatten(std::vector<IState*>& states) {
    std::set<IState*> added; // use set to prevent duplicates

    interfaces::apply<IState*>(states, [&added](IState* state) -> void {
        if ( state->type() == IState::Type::SET ) {
            auto f = __flatten(((StateSet*)state)->_states);
            added.insert(f.begin(), f.end());
        } else if ( !added.contains(state) ) {
            added.insert(useref(state));
        }
    });

    std::vector<IState*> out(added.begin(), added.end());
    return out;
}

bool StateSet::__compareStates(IState* s1, IState* s2) {
    return *s1 < *s2;
}

}
