#ifndef YUNOLEX_REGEXPARSER_H
#define YUNOLEX_REGEXPARSER_H

#include <algorithm>
#include <stdexcept>
#include "../abstractregex.h"
#include "../framework/dbg.h"

namespace yunolex {

class ParserException final : public std::exception {
public:
    ParserException(std::string what, int l, int c) : _what(what + "[" + std::to_string(l) + "," + std::to_string(c) + "]") {}
    const char* what() const noexcept override { return _what.c_str(); }
private:
    std::string _what;
};

class RegexParser final {
public:
    static Node* parse(std::string input, int sl, int sc) {
        RegexParser p(input, sl, sc);

        return p.parseRegex();
    }
private:
    struct IntervalData {
        IntervalData() : lower(0), upper(-1), comma(false) {}
        int lower;
        int upper;
        bool comma;
    };

    RegexParser(std::string input, int startingLine, int startingColumn) : _input(input), _line(startingLine), _col(startingColumn), _index(0) {
        if ( _input.size() == 0 ) {
            throw ParserException("Unexpected EOF", _line, _col);
        }
    }

    ~RegexParser() = default;

    [[nodiscard]] Node* parseRegex() {
        auto re = concat();
        info(std::cout, "parseRegex got " + re->toString() + " from concat!", true);
        if ( _index == _input.size() ) return re;
        char c = _input[_index];
        if ( c == '|' ) {
            _index++;
            return new Alternation(re, parseRegex());
        }
        if ( c == ')' ) return re;
        throw ParserException("Bad regex, expected alternation or eof", _line, _col);
    }

    [[nodiscard]] Node* concat() {
        auto re = basicre();
        info(std::cout, "concat got " + re->toString() + " from basicre!", true);
        if ( _index == _input.size() || _input[_index] == '|' || _input[_index] == ')' ) return re;
        return new Concatenation(re, concat());
    }

    [[nodiscard]] Node* basicre() {
        auto elem = elemre();
        info(std::cout, "basicre got " + elem->toString() + " from elemre!", true);
        char c = _input[_index];
        while ( c == '*' || c == '+' || c == '?' || c == '{' ) {
            _index++;
            if ( elem->name() == "Interval" ) {
                auto in = (Interval*)elem;
                if ( in->upper() != 0 ) { // modifiying nothing yeilds nothing
                    if ( c == '*' ) { // (regex){x,y}*
                        if ( in->lower() < 2 ) { // choose 1 or 0 from interval every time == star, greater numbers subsumed by star
                            elem = new Star(in->body());
                            delete in;
                        } else {
                            elem = new Star(elem);
                        }
                    } else if ( c == '+' ) { // (regex){x,y}+
                        if ( in->lower() < 2 ) { // choose 1 from interval every time == plus, greater numbers subsumed by plus
                            elem = new Plus(in->body());
                            delete in;
                        } else {
                            elem = new Plus(elem);
                        }
                    } else if ( c == '?' ) {
                        if ( in->lower() == 1 ) { // if lower bound is 1, add 0 to bounds
                            elem = new Interval(in->body(), 0, in->upper());
                            delete in;
                        } else if ( in->lower() > 1 ) { // interval has gap, have to wrap
                            elem = new Question(elem);
                        }
                    } else if ( c == '{' ) {
                        auto id = parseIntervalData();
                        if ( id.lower == 0 && id.upper == 1 ) {
                            elem = new Question(elem);
                        } else if ( id.lower == 0 && id.upper == -1 ) {
                            elem = new Star(elem);
                        } else if ( id.lower == 1 && id.upper == -1 ) {
                            elem = new Plus(elem);
                        } else {
                            elem = new Interval(elem, id.lower, id.comma ? id.upper : id.lower);
                        }
                    }
                }
            } else if ( elem->name() != "Star" ) {
                if ( c == '*' ) {
                    if ( elem->name() == "Plus" ) { // a+* == a*
                        auto temp = (Plus*)elem;
                        elem = useref(temp->left());
                        delete temp;
                    }
                    else if ( elem->name() == "Question" ) { // a?* == a*
                        auto temp = (Question*)elem;
                        elem = useref(temp->body());
                        delete temp;
                    }
                    elem = new Star(elem);
                } else if ( c == '+' ) {
                    if ( elem->name() == "Question" ) { // a?+ == a*
                        auto temp = (Question*)elem;
                        elem = new Star(temp->body());
                        delete temp;
                    } else if ( elem->name() != "Plus" ) { // a++ == a+
                        elem = new Plus(elem);
                    }
                } else if ( c == '?' ) {
                    if ( elem->name() == "Plus" ) { // a+? == a*
                        auto temp = (Plus*)elem;
                        elem = useref(temp->right());
                        delete temp;
                    } else if ( elem->name() != "Question" ) { // a?? == a?
                        elem = new Question(elem);
                    }
                } else if ( c == '{' ) {
                    auto id = parseIntervalData();
                    if ( elem->name() == "Plus" ) { 
                        if ( id.lower == 0 ) { // a+{0,x} == a*
                            auto temp = (Plus*)elem;
                            elem = useref(temp->right());
                            delete temp;
                        } // a+{y,x} == a+  (y!=0)
                    } else if ( elem->name() == "Question" ) {
                        auto temp = (Question*)elem;
                        if ( id.upper != 1 ) { // a?{x,1} == a?
                            if ( id.comma && id.upper == -1 ) { // a?{x,} == a*
                                elem = new Star(temp->body());
                            } else { // a?{x,y} == a{0,y}
                                elem = new Interval(temp->body(), 0, id.comma ? id.upper : id.lower);
                            }
                            delete temp;
                        }
                    } else {
                        if ( id.lower == 0 && id.upper == 1 ) {
                            elem = new Question(elem);
                        } else if ( id.lower == 0 && id.upper == -1 && id.comma ) {
                            elem = new Star(elem);
                        } else if ( id.lower == 1 && id.upper == -1 && id.comma ) {
                            elem = new Plus(elem);
                        } else {
                            elem = new Interval(elem, id.lower, id.comma ? id.upper : id.lower);
                        }
                    }
                }
            } else if ( c == '{' ) {
                parseIntervalData(); // case where we interval a star
            }
            c = _input[_index];
        }
        return elem;
    }

    [[nodiscard]] Node* elemre() {
        char c = _input[_index++];
        std::string dbgstr(1, c);
        info(std::cout, "elemre character: " + dbgstr, true);
        if ( c == '(' ) return group();
        if ( c == '[' ) return charSelect();
        if ( c == '.' ) return new Wildcard();
        if ( c == '\\' ) {
            c = _input[_index++];
            if ( c == 'n' ) return new Symbol('\n');
            if ( c == 't' ) return new Symbol('\t');
            
        }
        return new Symbol(c);
    }

    [[nodiscard]] Node* group() {
        auto n = parseRegex();
        if ( _input[_index] == ')') _index++;
        else throw ParserException("Invalid Group, expected ')'", _line, _col);
        return n;
    }

    [[nodiscard]] Node* charSelect() {
        bool neg = false;
        if ( _input[_index] == '^' ) {
            _index++;
            neg = true;
        }
        std::set<char> options; 
        char prev = -1;
        while ( _input[_index] != ']' && _index != _input.size() ) {
            char c = _input[_index++];
            if ( c == '\\' ) {
                char c2 = _input[_index++];
                if ( c2 == 'n' ) options.insert('\n');
                else if ( c2 == 't' ) options.insert('\t');
                else options.insert(c2);
            } else if ( c == '-' ) {
                if ( prev == -1 || _input[_index] == ']' ) options.insert(c);
                else if ( prev > _input[_index] ) {
                    std::string msg = "Bad range: " + std::to_string(prev) + "-" + std::to_string((char)_input[_index]);
                    throw ParserException(msg, _line, _col);
                }
                else {
                    char upper = _input[_index++];
                    for (int i = prev; i < upper + 1; i++) {
                        options.insert(i);
                    }
                }
            } else {
                options.insert(c);
            }
            prev = c;
        }
        CharacterSelect* charsel;
        if ( neg ) {
            // get everything else
            std::set<char> diff;
            Wildcard all;
            all._options.insert('\n');
            std::set_difference(all._options.begin(), all._options.end(), options.begin(), options.end(), std::inserter(diff, diff.end()));
            options = diff;
        }
        charsel = new CharacterSelect(options);
        if ( _input.size() == _index ) throw ParserException("Unexpected EOF, expected ']'", _line, _col);
        _index++;
        return charsel;
    }

    IntervalData parseIntervalData() {
        IntervalData d;
        char c;
        while ( (c = _input[_index++]) != '}' ) {
            if ( c == ',' ) {
                d.comma = true;
                if ( _input[_index] != '}' ) d.upper = 0;
                continue;
            }
            if ( c < 48 || c > 57 ) throw ParserException("Unexpected " + std::to_string((char)c) + ", expected digit", _line, _col);
            if ( d.comma ) {
                d.upper = d.upper * 10 + c - '0';
            } else {
                d.lower = d.lower * 10 + c - '0';
            }
        }
        return d;
    }

private:
    std::string _input;
    std::size_t _line, _col, _index;
};

}

#endif