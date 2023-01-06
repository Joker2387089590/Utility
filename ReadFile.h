#pragma once
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

/// read content of a file with the help of std only
namespace ReadFiles
{
namespace fs = std::filesystem;

namespace Detail
{
inline std::ifstream getStream(const fs::path& file)
{
	std::ifstream stream(file);
	if(!stream.is_open())
		throw std::runtime_error("can't open file:" + file.string());
	return stream;
}
} // namespace Detail

inline std::vector<char> readFile(const fs::path& file)
{
	auto size = fs::file_size(file);
	if(size == 0) return {};
	auto stream = Detail::getStream(file);
	std::vector<char> content(size, '\0');
	stream.read(content.data(), size);
	return content;
}

// a text file never contains '\0' prefixes or suffixes
// NOTE: it's the caller who is responsible for dealing with the '\0's mixed.
inline std::string readText(const fs::path& file)
{
	auto size = fs::file_size(file);
	if(size == 0) return {};
	auto stream = Detail::getStream(file);
	std::string content(size, '\0');
	stream.read(content.data(), size);

	// erase suffix first
	auto bpos = content.find_last_not_of('\0'); // == npos if all zero
	content.erase(bpos + 1); // if pos == npos, pos + 1 == 0 (unsigned overflow is well defined)
	auto fpos = content.find_first_not_of('\0');
	if(fpos != content.npos) content.erase(0, fpos);

	return content;
}

inline std::vector<std::string> readLines(const fs::path& file)
{
	auto size = fs::file_size(file);
	if(size == 0) return {};
	auto stream = Detail::getStream(file);
	std::vector<std::string> lines;
	std::string tmp;
	while(std::getline(stream, tmp)) lines.push_back(std::move(tmp));
	return lines;
}
} // namespace ReadFiles
