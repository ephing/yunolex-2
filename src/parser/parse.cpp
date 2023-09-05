#include "parse.h"

namespace yunolex {

std::vector<Token*>* parseFile(std::string input) {
    std::ifstream infile;
    infile.open(input);

    if ( infile.bad() ) {
        throw ParserException("Unable to open input file: " + input);
    }

    std::vector<Token*>* tkstream = new std::vector<Token*>();
    std::string current;
    std::size_t line = 0;
    bool fail = false, skip = false;

    while ( std::getline(infile, current) ) {
        line++;
        parsehelp::trim(current);

        if ( current[0] == '[' ) { // start of new token
            skip = false;

            if ( current.back() != ']' ) {
                std::cerr << "Expected \']\' at [" << line << "," << current.size() << "]" << std::endl;
                fail = skip = true;
            }

            if ( tkstream->size() > 0 && !parsehelp::verifyToken(tkstream->back()) ) {
                fail = true;
            }

            tkstream->push_back(new Token());
            tkstream->back()->Name = current.substr(1, current.size() - 2);
            info(std::cout, "Created token " + tkstream->back()->Name);

        } else if ( skip ) { // we got an error parsing this token, skip lines until we find the next token
            continue;
        } else if ( current.substr(0,5) == "regex" ) {
            auto eq = current.find('=');
            auto regex = current.substr(eq + 1);
            auto s = regex.size();
            parsehelp::trim(regex);

            try {
                tkstream->back()->Regex = useref(RegexParser::parse(regex, line, eq + s + 1));
                info(std::cout, "Regex: " + regex + " -> " + tkstream->back()->Regex->toString());
            } catch (ParserException& p) {
                std::cerr << p.what() << std::endl;
                fail = skip = true;
            }

        } else if ( current.substr(0,2) == "in" ) {
            info(std::cout, "Parsing 'in' field of token " + tkstream->back()->Name);
            auto list = current.substr(current.find('=') + 1);
            parsehelp::trim(list);
            parsehelp::parseSet(list, tkstream->back()->In);

            if ( tkstream->back()->In.size() == 0 ) {
                std::cerr << "Tokens must be within at least 1 scope set [" << line << "," << current.find('=') + 1 << "]" << std::endl;
                fail = skip = true;
            }

        } else if ( current.substr(0,5) == "enter" ) { // parse enter field
            info(std::cout, "Parsing 'enter' field of token " + tkstream->back()->Name);
            auto list = current.substr(current.find('=') + 1);
            parsehelp::trim(list);
            parsehelp::parseSet(list, tkstream->back()->Enter);

        } else if ( current.substr(0,5) == "leave" ) { // parse leave field
            info(std::cout, "Parsing 'leave' field of token " + tkstream->back()->Name);
            auto list = current.substr(current.find('=') + 1);
            parsehelp::trim(list);
            parsehelp::parseSet(list, tkstream->back()->Leave);

        } else if ( current.substr(0,4) == "skip" ) { // parse skip field
            info(std::cout, "Parsing 'skip' field of token " + tkstream->back()->Name);
            auto sk = current.substr(current.find('=') + 1);
            parsehelp::trim(sk);
            tkstream->back()->Skip = sk == "true";

        } else if ( current.substr(0,5) == "error") { // parse error field
            tkstream->back()->Error = true;
            auto q = current.find('\"');
            if ( current.back() != '\"' ) {
                std::cerr << "Missing closing quotation mark at [" << line << "," << current.size() << "]" << std::endl;
                fail = skip = true;
            }
            if ( q == std::string::npos ) {
                std::cerr << "Error message not encapsulation in quotation marks at [" << line << "," << current.size() << "]" << std::endl;
                fail = skip = true;
            }
            if ( q == current.size() - 1 ) {
                std::cerr << "No opening quotation mark for error message at [" << line << "," << current.find('=') + 1 << "]" << std::endl;
                fail = skip = true;
            }

            tkstream->back()->ErrorMsg = current.substr(q + 1, current.size() - q - 2);
        }
    }

    infile.close();

    if ( fail ) {
        throw ParserException("Token specification parsing failed!");
    }

    return tkstream;
}

namespace parsehelp {

void parseSet(std::string line, std::set<std::string>& set) {
    while ( true ) {
        info(std::cout, "\tline: " + line + " , size: " + std::to_string(line.size()));
        auto delimiter = line.find(' ');
        if ( delimiter == std::string::npos ) {
            set.insert(line);
            break;
        }
        auto scope = line.substr(0,delimiter);
        set.insert(scope);
        info(std::cout, "\tAdded " + scope + " to set");
        line = line.substr(delimiter + 1);
        trim(line);
    }
}

bool verifyToken(Token* token) {
    bool good = true;
    if ( token->Regex == nullptr ) { 
        std::cerr << token->Name << " does not have a regular expression specification!\n";
        good = false;
    }
    if ( token->In.size() == 0 ) { // scopes with no "In" spec default to OUTERSCOPE
        token->In.insert(OUTERSCOPE);
        info(std::cout, "Added $ to scopes of token " + token->Name);
    }
    return good;
}

inline void trim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));

    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

}
}