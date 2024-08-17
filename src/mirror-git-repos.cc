#include <iostream>
#include <fstream>
#include <memory>
#include <numeric> // std::reduce
#include <regex>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm> // std::for_each
#include <functional> // std::bind_front
#include <thread>
#include <mutex>
#include <cstdlib>
#include <cstdio>

namespace std
{
namespace fs = filesystem;
}

std::mutex errors_mutex;

#define INFO_TO(o, x) std::o << "[" << path.string() << "]" << x << std::endl

bool run_in_path(const std::fs::path path, const char *command)
{
	const static auto pattern = std::regex("\\n");
	const std::string cmd =
		std::string("cd ") + path.string() + " && " + command + " 2>&1";

#ifdef DEBUG
	INFO_TO(cout, "Running command: " << cmd);
#endif

	FILE *cmd_output;
	if ((cmd_output = popen(cmd.c_str(), "r")) == nullptr) {
		return false;
	}
	const auto buf = std::make_unique<char[]>(512);
	while (std::fgets(buf.get(), 512, cmd_output) != nullptr) {
		if (buf != nullptr) {
			auto line = std::string(buf.get());
			INFO_TO(cout,
				" (CMD \"" << command << "\"): " << std::regex_replace(line, pattern, ""));
		}
	}
	const auto exit_code = pclose(cmd_output) % 255;
	return static_cast<bool>(!exit_code);
}

void update_repo(const std::fs::path path, std::vector<std::size_t> *errors)
{
#define ERR(x) INFO_TO(cout, " (ERR): " << x)
#define OK(x) INFO_TO(cout, " (OK): " << x)

	std::size_t error_count = 0;

	const auto run_in_repo_dir = std::bind_front(run_in_path, path);

	if (std::fs::is_directory(path)) {
		if (!run_in_repo_dir("git remote update")) {
			ERR("update failed");
			error_count++;
		} else {
			OK("update successful");
		}
		if (!run_in_repo_dir("git gc")) {
			ERR("gc failed");
			error_count++;
		} else {
			OK("gc successful");
		}
		if (!run_in_repo_dir("git lfs fetch --all")) {
			ERR("lfs update failed");
			error_count++;
		} else {
			OK("lfs update successful");
		}
	} else {
		std::string repo_url;
		auto f = std::ifstream(path);
		if (!f.is_open()) {
			ERR("failed to open file");
			error_count++;
		} else {
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
		} else {
			std::fs::remove(path.string() + ".bak");
		}
		auto errs = std::vector<std::size_t>();
		errs.reserve(1);
		update_repo(path, &errs);
		error_count += errs[0];
	}

end:
	auto lock = std::scoped_lock(errors_mutex);
	if (error_count > 0) {
		ERR("finished, " << error_count << " erorrs");
	} else {
		OK("finished, " << error_count << " erorrs");
	}
	errors->push_back(error_count);
	return;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		std::cerr << "not enough arguments" << std::endl;
		std::exit(1);
	}
	auto mirrors_dir = std::fs::path(argv[1]);
	auto jobs = std::vector<std::thread>();
	auto errors = std::vector<std::size_t>();
	auto iter = std::fs::directory_iterator(mirrors_dir);
	std::for_each(std::fs::begin(iter), std::fs::end(iter),
		      [&jobs, &errors](auto e) {
			      jobs.push_back(
				      std::thread(update_repo, e, &errors));
		      });
	std::for_each(jobs.begin(), jobs.end(), [](auto &e) { e.join(); });
	std::size_t total_errors =
		std::reduce(errors.begin(), errors.end(), 0,
			    [](auto a, auto e) { return a + e; });
	std::cout << "All jobs finished, total errors: " << total_errors
		  << std::endl;

	if (total_errors == 255) {
		return total_errors;
	} else {
		return total_errors % 255;
	}
}
