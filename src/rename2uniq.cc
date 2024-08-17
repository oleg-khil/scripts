#include <filesystem>
#include <iostream>
#include <regex>
#include <string>

#include "utils.hh"

#include <uuid.h>

int main(int argc, char** argv) {
	if (argc < 2) {
		return exit_codes::wrong_arguments;
	}
	std::mt19937 prng_generator;
	{
		std::random_device rd;
		auto seed_data = std::array<int, std::mt19937::state_size>{};
		std::generate(std::begin(seed_data), std::end(seed_data),
			      std::ref(rd));
		auto seq = std::seed_seq(std::begin(seed_data),
					 std::end(seed_data));
		prng_generator = std::mt19937{ seq };
	}
	auto uuid_generator = uuids::uuid_random_generator{ prng_generator };

	for (int i = 1; i < argc; i++) {
		const auto file = std::filesystem::path(argv[i]);
		const auto ext = utils::get_ext_lowercase(file.filename());
		const uuids::uuid id = uuid_generator();
		const std::string new_filename =
			std::regex_replace(uuids::to_string(id),
					   utils::make_regexp("-{1}"), "") +
			ext;
		const auto new_file = file.parent_path() / new_filename;
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
