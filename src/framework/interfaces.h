#ifndef FRAMEWORK_INTERFACES_H
#define FRAMEWORK_INTERFACES_H

#include <string>
#include <stdexcept>
#include <set>
#include <functional>
#include <type_traits>
#include <ostream>

namespace interfaces {

    class Stringable {
    public:
        virtual ~Stringable() = default;
        [[nodiscard]] virtual std::string toString() const = 0;
    };

    class InvalidFree : public std::exception {
    public:
        InvalidFree() = default;
        virtual ~InvalidFree() = default;

        const char* what() const noexcept override { return "Attempt to free an object with 0 references"; }
    };

    class BadReduce : public std::exception {
    public:
        BadReduce() = default;
        virtual ~BadReduce() = default;

        const char* what() const noexcept override { return "Attempt to reduce an iterable with 0 elements."; }
    };


    template <typename T>
    concept Iterable = requires(T& v) {
        { v.begin() } -> std::convertible_to<typename T::iterator>;
        { v.end() } -> std::convertible_to<typename T::iterator>;
        { v.size() } -> std::convertible_to<std::size_t>;
    };

    // This is legit just a glorified foreach loop, but it looks cooler so I'm sticking with it
    template <typename T>
    void apply(Iterable auto iterable, std::function<void(T)> callback) {
        for ( auto s : iterable ) callback(s);
    }

    template <typename T>
    T reduce(const Iterable auto iterable, std::function<T(T, T)> callback) {
        if ( !iterable.size() ) throw BadReduce();
        T sum = *iterable.begin();        
        for ( auto s = iterable.begin() + 1; s != iterable.end(); s++ ) {
            sum = callback(sum, *s);
        }
        return sum;
    }

    class Reference {
    public:
        Reference() : _count(0) {
            Reference::_references.insert(this);
        }

        virtual ~Reference() {
            Reference::_references.erase(this);
        }

        std::size_t count() const {
            return _count;
        }

        void incRef() {
            _count += 1;
        }

        void decRef() {
            if ( _count == 0 ) throw InvalidFree();
            _count--;
            if ( _count == 0 ) delete this;
        }

        static bool exists(Reference* ref) {
            return _references.contains(ref);
        }

        static void dump(std::ostream& out) {
            out << "Reference Dump:" << std::endl;
            for ( auto i : _references ) {
                out << "\t" << dynamic_cast<Stringable*>(i)->toString() << " " << i->_count << std::endl;
            }
            out << std::endl;
        }

    private:
        std::size_t _count;
        static std::set<Reference*> _references;
    };

    template <typename T>
    concept Referencable = requires(T& v) {
        { std::is_base_of_v<Reference, T> };
    };

    auto useref(Referencable auto ref) {
        if ( ref == nullptr ) return ref;
        ref->incRef();
        return ref;
    }

    void freeref(Referencable auto ref) {
        if ( ref == nullptr ) return;
        ref->decRef();
    }
}

#endif