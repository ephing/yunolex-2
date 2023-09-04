#include "dbg.h"
#include <ostream>

namespace yunolex {

void info(std::ostream& out, std::string message, bool verbose) {
#ifdef DEBUG
#ifndef VERBOSE
    if ( !verbose )
#endif
    out << " [\033[34mINFO\033[0m] " << message << std::endl;
#endif
}

void warning(std::ostream& out, std::string message, bool verbose) {
#ifdef DEBUG
#ifndef VERBOSE
    if ( !verbose )
#endif
    out << " [\033[33mWARNING\033[0m] " << message << std::endl;
#endif
}

void error(std::ostream& out, std::string message, bool verbose) {
#ifdef DEBUG
#ifndef VERBOSE
    if ( !verbose )
#endif
    out << " [\033[33mERROR\033[0m] " << message << std::endl;
#endif
}

}