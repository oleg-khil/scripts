#ifndef _UTILS_HH
#define _UTILS_HH
#include <regex>
#include <string>

namespace utils
{

	std::regex make_regexp(const std::string& regexp) {
		return std::regex(regexp, std::regex_constants::ECMAScript |
						  std::regex_constants::icase);
	}

	const auto file_extension_regex = make_regexp("\\.([A-Z\\d]*)$");

	std::string get_ext(const std::string& filename) {
		std::smatch ext_match;
		std::regex_search(filename, ext_match,
				  file_extension_regex); // get ext via regex
		return ext_match[0].str(); // return ext as str
	}
	std::string get_ext(const char* filename) {
		return get_ext(std::string(filename));
	}

	std::string get_ext_lowercase(const std::string filename) {
		auto ext = get_ext(filename);
		std::transform(ext.begin(), ext.end(), ext.begin(),
			       [](unsigned char c) { return std::tolower(c); });
		return ext;
	}
	std::string get_ext_lowercase(const char* filename) {
		return get_ext_lowercase(std::string(filename));
	}

} // namespace utils

namespace exit_codes
{
	enum { success = 0, wrong_arguments, fail };
} // namespace exit_codes

#endif
