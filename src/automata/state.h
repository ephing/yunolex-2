#ifndef YUNOLEX_STATE_H
#define YUNOLEX_STATE_H

#include <vector>
#include "../framework/interfaces.h"

namespace yunolex {

#define EPS "ε"

class Transition;

// If this gets thrown, theres an oopsie somewhere
class InvalidStateSet : public std::exception {
public:
    InvalidStateSet() = default;
    virtual ~InvalidStateSet() = default;

    [[nodiscard]] const char* what() const noexcept override { return "Attempt to reduce a StateSet with 0 members"; }
};

class IState : public interfaces::Stringable, public interfaces::Reference {
public:
    virtual ~IState();
    enum class Type { SINGLETON, SET };
    [[nodiscard]] std::set<Transition*> outbound() const { return _outbound; }
    [[nodiscard]] std::string toString() const override { return _id; }
    [[nodiscard]] bool isFinal() const { return _final; }
    [[nodiscard]] Type type() const { return _type; }
    void addEdge(IState*, std::string);
    void removeEdge(Transition*);
    [[nodiscard]] bool containsEdge(IState*, std::string) const;
    [[nodiscard]] IState* nextState(std::string) const;
    [[nodiscard]] bool semanticallyEquivalent(IState*) const;
    void setFinal(bool f) { _final = f; }
    [[nodiscard]] bool operator<(IState& other) { return _id.substr(1).compare(other._id.substr(1)) < 0; }
    [[nodiscard]] bool operator==(IState& other) { return _id.substr(1) == other._id.substr(1); }

    // returns transitivereflexive closure of the state
    // if parameter is true, only follows epsilon transitions
    [[nodiscard]] std::set<const IState*> transitiveReflexiveClosure(bool) const;
protected:
    IState() = delete;
    explicit IState(std::string id, bool fin, Type type) : _id(id), _final(fin), _type(type) {}

    std::set<Transition*> _outbound;
    std::string _id;
    bool _final;
    Type _type;
private:
    void __trClosure(std::set<const IState*>&, bool) const;
};

class State final : public IState {
public:
    explicit State(bool fin) : IState("s_q" + std::to_string(Id++) + "_", fin, IState::Type::SINGLETON) {}
protected:
    static std::size_t Id;
};

class StateSet : public IState {
public:
    explicit StateSet(std::vector<IState*>);
    virtual ~StateSet();

    // higher order functions go brrrr
    template <typename T>
    std::vector<T> map(std::function<T(IState*)> callback) {
        std::vector<T> out;
        for ( auto s : _states ) {
            out.push_back(callback(s));
        }
        return out;
    }
protected:
    std::vector<IState*> _states;
private:
    [[nodiscard]] static std::vector<IState*> __flatten(std::vector<IState*>&);
    [[nodiscard]] static bool __compareStates(IState*, IState*);
};

class Transition : public interfaces::Stringable, public interfaces::Reference {
public:
    explicit Transition(std::string symbol, IState* src, IState* dest) : _symbol(symbol), _src(src), _dest(dest) {}

    ~Transition() = default;

    [[nodiscard]] std::string toString() const override {
        return _src->toString() + " ->" + _symbol + " " + _dest->toString();
    }

    enum class Type { EPSILON, NORMAL };

    [[nodiscard]] virtual Transition::Type getType() const { return Transition::Type::NORMAL; }

    [[nodiscard]] IState* source() const { return _src; }
    [[nodiscard]] IState* dest() const { return _dest; }
    [[nodiscard]] std::string symbol() const { return _symbol; }
protected:
    std::string _symbol;

    IState* _src;
    IState* _dest;
};

class EpsilonTransition final : public Transition {
public:
    explicit EpsilonTransition(IState* src, IState* dest) : Transition(EPS, src, dest) {}
    ~EpsilonTransition() = default;

    [[nodiscard]] Transition::Type getType() const override { return Transition::Type::EPSILON; }
};

}

#endif