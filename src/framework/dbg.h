#ifndef YUNOLEX_DBG_H
#define YUNOLEX_DBG_H

#include <ostream>

namespace yunolex {

/**
 * Info Diagnostic
 * if `verbose`, message won't print in DEBUG mode unless also in VERBOSE mode
 */
void info(std::ostream& out, std::string message, bool verbose = false);

/**
 * Warning Diagnostic
 * if `verbose`, message won't print in DEBUG mode unless also in VERBOSE mode
 */
void warning(std::ostream& out, std::string message, bool verbose = false);

/**
 * Error Diagnostic
 * if `verbose`, message won't print in DEBUG mode unless also in VERBOSE mode
 */
void error(std::ostream& out, std::string message, bool verbose = false);

}

#endif
