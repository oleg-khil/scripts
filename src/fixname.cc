#include <algorithm>
#include <filesystem>
#include <iostream>
#include <regex>
#include <string>

#include "utils.hh"

std::string fix_filename(const std::string& filename) {
	auto fixed = std::string(filename);
	fixed = std::regex_replace(fixed, utils::make_regexp("^-*"), "");
	fixed = std::regex_replace(
		fixed, utils::make_regexp("[\\[\\]\\s\\$\\?]"), "_");

	const auto ext = utils::get_ext_lowercase(filename);
	fixed = std::regex_replace(fixed, utils::file_extension_regex, ext);
	return fixed;
}
std::string fix_filename(const char* filename) {
	return fix_filename(std::string(filename));
}

int main(int argc, char** argv) {
	if (argc < 2) {
		return exit_codes::wrong_arguments;
	}
	for (int i = 1; i < argc; i++) {
		const auto file = std::filesystem::path(argv[i]);
		const auto new_file =
			file.parent_path() / fix_filename(file.filename());
		if (file.filename() == new_file.filename()) {
			continue;
		}
		try {
			std::filesystem::rename(file, new_file);
		}
		catch (...) {
			std::cerr << "Failed to rename \"" << file.string()
				  << "\" to \"" << new_file.string() << "\""
				  << std::endl;
			return exit_codes::fail;
		}
		std::cout << file.string() << " -> " << new_file.string()
			  << std::endl;
	}
	return exit_codes::success;
}
