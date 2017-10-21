#include "all.h"
#include "regex.h"


/* this code is in its own compilation unit because it literally takes several
 * seconds for the template instantiation to occur during each compile, and
 * that's just ridiculous */


static std::basic_regex<char> *filter = NULL;


void r_init(const char *pattern)
{
	if (filter != NULL) {
		delete filter;
	}
	
	filter = new std::basic_regex<char>(pattern,
		std::regex::icase | std::regex::optimize | std::regex::ECMAScript);
}


bool r_match(const char *input)
{
	assert(filter != NULL);
	
	return std::regex_match(input, *filter, std::regex_constants::match_any);
}
