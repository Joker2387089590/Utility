#pragma once
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

namespace ReadFiles
{
namespace fs = std::filesystem;

inline std::vector<char> readFile(const fs::path& file)
{
	if(!fs::exists(file)) return {};

	std::ifstream stream(file);
	if(!stream.is_open()) return {};

	auto size = fs::file_size(file);
	std::vector<char> content(size);
	stream.read(content.data(), size);
	return content;
}

inline std::vector<std::string> readLines(const fs::path& file)
{
	if(!fs::exists(file)) return {};

	std::ifstream stream(file);
	if(!stream.is_open()) return {};

	std::vector<std::string> lines;
	std::string tmp;
	while(std::getline(stream, tmp)) lines.push_back(std::move(tmp));
	return lines;
}

}
