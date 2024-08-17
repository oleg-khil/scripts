#include <algorithm> // std::for_each
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional> // std::bind_front
#include <future>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <vector>

namespace std
{
	namespace fs = filesystem;
} // namespace std

#define INFO_TO(o, x) std::o << "[" << path.string() << "]" << x << std::endl

bool run_in_path(const std::fs::path path, const char* command) {
	const static auto pattern = std::regex("\\n");
	const std::string cmd =
		std::string("cd ") + path.string() + " && " + command + " 2>&1";

#ifdef DEBUG
	INFO_TO(cout, "Running command: " << cmd);
#endif

	FILE* cmd_output;
	if ((cmd_output = popen(cmd.c_str(), "r")) == nullptr) {
		return false;
	}
	const auto buf = std::make_unique<char[]>(512);
	while (std::fgets(buf.get(), 512, cmd_output) != nullptr) {
		if (buf != nullptr) {
			auto line = std::string(buf.get());
			INFO_TO(cout, " (CMD \"" << command << "\"): "
						 << std::regex_replace(
							    line, pattern, ""));
		}
	}
	const auto exit_code = pclose(cmd_output) % 255;
	return static_cast<bool>(!exit_code);
}

std::size_t update_repo(const std::fs::path path) {
#define ERR(x) INFO_TO(cout, " (ERR): " << x)
#define OK(x) INFO_TO(cout, " (OK): " << x)

	std::size_t error_count = 0;

	const auto run_in_repo_dir = std::bind_front(run_in_path, path);

	if (std::fs::is_directory(path)) {
		if (!run_in_repo_dir("git remote update")) {
			ERR("update failed");
			error_count++;
		}
		else {
			OK("update successful");
		}
		if (!run_in_repo_dir("git gc")) {
			ERR("gc failed");
			error_count++;
		}
		else {
			OK("gc successful");
		}
		if (!run_in_repo_dir("git lfs fetch --all")) {
			ERR("lfs update failed");
			error_count++;
		}
		else {
			OK("lfs update successful");
		}
	}
	else {
		std::string repo_url;
		auto f = std::ifstream(path);
		if (!f.is_open()) {
			ERR("failed to open file");
			error_count++;
		}
		else {
			std::getline(f, repo_url);
		}
		f.close();
		std::fs::rename(path, std::fs::path(path.string() + ".bak"));
		if (!run_in_path(path.parent_path(),
				 ("git clone --mirror " + repo_url + " " +
				  path.filename().string())
					 .c_str())) {
			ERR("initial mirror clone failed");
			error_count++;
			std::fs::rename(path.string() + ".bak", path);
			goto end;
		}
		else {
			std::fs::remove(path.string() + ".bak");
		}
		error_count += update_repo(path);
	}

end:
	if (error_count > 0) {
		ERR("finished, " << error_count << " erorrs");
	}
	else {
		OK("finished, " << error_count << " erorrs");
	}
	return error_count;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "not enough arguments" << std::endl;
		std::exit(1);
	}
	auto mirrors_dir = std::fs::path(argv[1]);
	auto jobs = std::vector<std::future<std::size_t>>();
	std::atomic_size_t errors = 0;
	auto iter = std::fs::directory_iterator(mirrors_dir);
	std::for_each(std::fs::begin(iter), std::fs::end(iter),
		      [&jobs](auto e) {
			      jobs.push_back(std::async(std::launch::async,
							update_repo, e));
		      });
	std::for_each(jobs.begin(), jobs.end(),
		      [&errors](auto& e) { errors += e.get(); });
	std::cout << "All jobs finished, total errors: " << errors << std::endl;

	if (errors == 255) {
		return errors;
	}
	else {
		return errors % 255;
	}
}
