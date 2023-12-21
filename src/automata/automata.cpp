#include "automata.h"
#include <map>

namespace yunolex {

void Automata::dot(std::ostream& out) const {
    out << "digraph automata" << this << " {\n\ts [shape=none,label=\"\"]\n";
    for (auto state : *_states) {
        out << "\t" << state->toString() << " [shape=";
        if ( state->isFinal() ) out << "double";
        out << "circle]\n";
    }
    out << "\ts -> " << _startState->toString() << " []\n";
    for ( auto state : *_states ) {
        for ( auto t : state->outbound() ) {
            auto s = (t->symbol() == "\\" || t->symbol() == "\"") ? "\\" + t->symbol() : t->symbol();
            out << "\t" << state->toString() << " -> " << t->dest()->toString() << " [label=\"" << s << "\"]\n";
        }
    }
    out << "}\n";
}

void Automata::removeEpsilonTransitions() {
    interfaces::apply<IState*>(*_states, [](IState* state) -> void {
        auto eps = state->transitiveReflexiveClosure(true); // get epsilon closure of state
        for ( auto s : eps ) {
            for ( auto t : s->outbound() ) {
                if ( t->getType() == Transition::Type::EPSILON ) { // remove epsilon transitions
                    state->setFinal(t->dest()->isFinal() || state->isFinal());
                    state->removeEdge(t);
                } else if ( s != state && !state->containsEdge(t->dest(), t->symbol()) ) { // dont add duplicate transitions from state
                    state->addEdge(t->dest(), t->symbol());
                }
            }
        }
    });
    __removeUnreachable();
}

void Automata::DFAify() {
    removeEpsilonTransitions();

    std::vector<IState*> svec;
    svec.push_back(_startState);
    auto startState = new StateSet(svec); // wrap start state in a set

    auto states = new std::set<IState*>(); // set of the new states that will be generated
    states->insert(useref(startState));

    __dfaHelp(startState, states);

    interfaces::apply<IState*>(*_states, [](IState* s) -> void { freeref(s); });
    _startState = startState;
    delete _states;
    _states = states;
    _finStates.clear();
    for ( auto s : *_states ) {
        if ( s->isFinal() ) _finStates.insert(s);
    }
}

void Automata::__dfaHelp(StateSet* state, std::set<IState*>* visited) {
    // unify all transitions from ^^^ with the same transition symbol, also track finality
    std::map<std::string, std::vector<IState*>> stateprep;
    // acquire transitions from all IStates in current StateSet
    auto transitions = state->map<std::set<Transition*>>([](IState* state) -> std::set<Transition*> {
        return state->outbound(); // using map to expose protected data go brrrr
    });
    // populate stateprep
    for ( auto tset : transitions ) {
        for ( auto t : tset ) {
            if ( !stateprep.count(t->symbol()) ) {
                stateprep.insert({ t->symbol(), std::vector<IState*>() });
            }
            stateprep.at(t->symbol()).push_back(t->dest());
        }
    }
    
    for ( auto trans : stateprep ) {
        auto newstate = new StateSet(trans.second);
        if ( auto s = Automata::containsState(*visited, newstate) ) { // if state already exist, use original
            state->addEdge(s, trans.first);
            delete newstate;
        } else { // create new state and recurse
            state->addEdge(newstate, trans.first);
            visited->insert(useref(newstate));
            __dfaHelp(newstate, visited);
        }
    }
}

void Automata::__removeUnreachable() {
    auto unvis = *_states;
    __rUHelp(_startState, unvis);
    for ( auto i : unvis ) {
        _states->erase(i);
        if ( _finStates.contains(i) ) _finStates.erase(i);
        freeref(i);
    }
}

void Automata::__rUHelp(IState* state, std::set<IState*>& unvisited) {
    unvisited.erase(state);

    for ( auto e : state->outbound() ) {
        if ( unvisited.contains(e->dest()) ) {
            __rUHelp(e->dest(), unvisited);
        }
    }
}

void Automata::concatenateSubsume(Automata* other) {
    interfaces::apply<IState*>(finstates(), [other](IState* state) -> void {
        state->addEdge(other->startState(), EPS);
    });
    clearFinal();
    assumeStates(other->states());
}

void Automata::minimize() {
    bool done;
    do {
        done = true;

        for ( auto s1 : *_states ) {
            for ( auto s2 : *_states ) {
                if ( s2 == _startState || s1 == s2 || (s1 < s2 && s1 != _startState) ) continue;
                if ( s1->semanticallyEquivalent(s2) ) {
                    done = false;

                    // its goin crazy around here parts, remapping incoming edges
                    for ( auto is : *_states ) {
                        if ( is == s2 ) continue;
                        for ( auto t : is->outbound() ) {
                            if ( t->dest() == s2 ) {
                                is->addEdge(s1, t->symbol());
                                is->removeEdge(t);
                            }
                        }
                    }

                    // remove s2
                    _states->erase(s2);
                    if ( s2->isFinal() ) _finStates.erase(s2);
                    freeref(s2);
                }
            }
        }
    } while ( !done );
}

// void Automata::__removeDead() {
//     auto states = *_states;
//     for ( auto s : states ) {
//         auto trc = s->transitiveReflexiveClosure(false);
//         bool canReachFinal = false;
//         for ( auto f : trc ) {
//             if ( f->isFinal() ) {
//                 canReachFinal = true;
//                 break;
//             }
//         } 
//         if ( !canReachFinal ) {
//             for ( auto j : *_states ) {
//                 for ( auto e : j->outbound() ) {
//                     if ( e->dest() == s ) {
//                         j->removeEdge(e);
//                     }
//                 }
//             }
//             _states->erase(s);
//             freeref(s);
//         }
//     }
// }

}
