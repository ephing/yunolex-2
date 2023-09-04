#ifndef YUNOLEX_DBG_H
#define YUNOLEX_DBG_H

#include <ostream>

namespace yunolex {

void info(std::ostream& out, std::string message, bool verbose = false);

void warning(std::ostream& out, std::string message, bool verbose = false);

void error(std::ostream& out, std::string message, bool verbose = false);

}

#endif
