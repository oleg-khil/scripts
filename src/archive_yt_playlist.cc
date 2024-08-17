#include <filesystem>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>

#if __cpp_lib_bind_front
#include <functional>
#endif

#include "utils.hh"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace std
{
	namespace fs = filesystem;
} // namespace std

#ifndef YT_DLP_CMD
#define YT_DLP_CMD "yt-dlp"
#endif

#ifndef YT_DLP_ARGS
#define YT_DLP_ARGS "-f bestaudio --audio-quality 321K -x --audio-format mp3"
#endif

std::tuple<int, std::string> run_command(std::string cmd) {
	std::string output;
	FILE* cmd_output_pipe;
	if ((cmd_output_pipe = popen(cmd.c_str(), "r")) == nullptr) {
		throw std::runtime_error("failed to open pipe");
	}
	const auto buf = std::make_unique<char[]>(512);

	while (std::fgets(buf.get(), 512, cmd_output_pipe) != nullptr) {
		if (buf != nullptr) {
			output += std::string(buf.get());
		}
	}
	return std::tuple<int, std::string>(
		static_cast<int>(pclose(cmd_output_pipe) % 255), output);
}

std::tuple<int, std::string> run_command_w_warn(std::string cmd) {
	auto result = run_command(cmd);
	auto exit_code = std::get<0>(result);
	if (exit_code) {
		std::cerr << "WARN: command (" << cmd
			  << ") completed with exit code " << exit_code
			  << std::endl;
	}
	return result;
}

bool file_exists_in_path_by_regex(std::fs::path path, std::regex regex) {
	for (auto file : std::fs::directory_iterator(path)) {
		if (std::regex_match(file.path().filename().c_str(), regex)) {
			return true;
		}
	}
	return false;
}

char* argv0;

void usage(void) {
	/* clang-format off */
    std::cout <<
        "Usage: " << argv0 << " <youtube/youtube music playlist url> <target directory>"
        << std::endl;
	/* clang-format on */
}

int main(int argc, char** argv) {
	argv0 = argv[0];
	if (argc != 3) {
		std::cerr << "ERROR: wrong number of arguments" << std::endl;
		usage();
		return exit_codes::wrong_arguments;
	}
	const auto playlist_url = std::string(argv[1]);
	const auto music_dir = std::string(argv[2]);
	json yt_dlp_output;
	{
		std::string yt_dlp_output_json;
		auto cmd = std::string(YT_DLP_CMD " " YT_DLP_ARGS
						  " -J --flat-playlist ") +
			   playlist_url;
		auto out = run_command_w_warn(cmd);
		yt_dlp_output = json::parse(std::get<1>(out));
	}

	if (!std::fs::exists(music_dir)) {
		std::cerr
			<< "WARN: target directory doesn't exists, creating... ";
		try {
			std::fs::create_directory(music_dir);
		}
		catch (...) {
			std::cerr << "ERROR" << std::endl;
			;
			return exit_codes::fail;
		}
		std::cerr << "OK" << std::endl;
	}
	std::fs::current_path(music_dir);

#if __cpp_lib_bind_front
	auto file_exists = std::bind_front(file_exists_in_path_by_regex,
					   std::fs::path(music_dir));
#else
	auto file_exists = [music_dir](std::regex regex) {
		return file_exists_in_path_by_regex(std::fs::path(music_dir),
						    regex);
	};
#endif

	std::cerr << "Legend:" << std::endl
		  << "\tF - found" << std::endl
		  << "\tN - not found" << std::endl;
	std::vector<json> missing;
	for (auto playlist_entry : yt_dlp_output["entries"]) {
		if (playlist_entry.is_null()) {
			continue;
		}
		auto id = static_cast<std::string>(playlist_entry["id"]);
		if (!file_exists(std::regex(std::string(".*") + id + ".*"))) {
			missing.push_back(playlist_entry);
		}
		else {
			std::cout << "[F] " << id << std::endl;
		}
	}

	if (missing.size() == 0) {
		return exit_codes::success;
	}

	std::string cmd = YT_DLP_CMD " " YT_DLP_ARGS;
	for (auto missing_el : missing) {
		std::cout << "[N] "
			  << static_cast<std::string>(missing_el["id"])
			  << std::endl;
		cmd += " " + static_cast<std::string>(missing_el["url"]);
	}

	std::cout << yt_dlp_output["entries"].size() - missing.size() << "/"
		  << yt_dlp_output["entries"].size() << " found, "
		  << missing.size() << " is missing, starting download"
		  << std::endl;

	run_command_w_warn(cmd);

	return exit_codes::
		success; // yt-dlp will exit with exit code 1 if something is deleted or unavailable for other reasons
}
