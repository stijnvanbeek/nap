// Core Includes
#include "stringutils.h"

// Std Includes
#include <algorithm>
#include <regex>
#include <sstream>
#include <cstring>
#include <locale>

namespace nap
{

	// Splits a string based on @inDelim
	void gSplitString(const std::string inString, char inDelim, std::vector<std::string>& ioParts)
	{
		// Clear
		ioParts.clear();

		// Create stream
		std::stringstream ss(inString);

		// Extract item and add
		std::string item;
		while (std::getline(ss, item, inDelim))
			ioParts.push_back(item);
	}


	// Write a string to ostream (binary)
	void gWriteString(std::ostream& stream, const std::string& text)
	{
		uint32_t size = text.size();

		stream.write(reinterpret_cast<const char*>(&size), sizeof(size));
		stream.write(text.data(), size);
	}


	// Read a string from istream (binary)
	std::string gReadString(std::istream& stream)
	{
		uint32_t size;
		stream.read(reinterpret_cast<char*>(&size), sizeof(size));

		std::string text(size, 0);
		stream.read(&text[0], size);

		return text;
	}


	// Converts a string to lower case characters
	void gToLower(std::string& ioString)
	{
		std::transform(ioString.begin(), ioString.end(), ioString.begin(), ::tolower);
	}


	// Converts @ioString to lower case characters, returns a copy
	std::string gToLower(const std::string& ioString)
	{
		std::string out_string = ioString;
		std::transform(out_string.begin(), out_string.end(), out_string.begin(), ::tolower);
		return out_string;
	}

	void gTokenize(const std::string& str, std::list<std::string>& tokens, const std::string& delims, bool omitTokens)
	{
		if (omitTokens) {
			std::regex re("([^" + delims + "]+)");
			std::regex_iterator<std::string::const_iterator> rit(str.begin(), str.end(), re);
			std::regex_iterator<std::string::const_iterator> rend;

			while (rit != rend) {
				tokens.push_back(rit->str());
				rit++;
			}
		}
		else {
			std::regex re("([" + delims + "]|[^" + delims + "]+)");
			std::regex_iterator<std::string::const_iterator> rit(str.begin(), str.end(), re);
			std::regex_iterator<std::string::const_iterator> rend;

			while (rit != rend) {
				tokens.push_back(rit->str());
				rit++;
			}
		}
	}


	std::string gStripNamespace(const std::string& str) {
		std::regex re("([^:]+)$");
		std::regex_iterator<std::string::const_iterator> rit(str.begin(), str.end(), re);
		std::regex_iterator<std::string::const_iterator> rend;
		if (rit != rend)
			return rit->str();
		return str;
	}

	
	bool gStartsWith(const std::string& inString, const std::string& inSubString, bool caseSensitive)
	{
		int(*compare_func)(const char*, const char*, size_t);
#ifdef _WIN32
		compare_func = caseSensitive ? &strncmp : &strnicmp;
#else
        compare_func = caseSensitive ? &strncmp : &strncasecmp;
#endif
		return compare_func(inString.c_str(), inSubString.c_str(), strlen(inSubString.c_str())) == 0;
	}


	bool gContains(const std::string& inString, const std::string& inSubString, bool caseSensitive)
	{
		// case sensitive
		if (caseSensitive)
			return inString.find(inSubString) != std::string::npos;

		// case in-sensitive
		std::locale loc;
		auto it = std::search(
			inString.begin(), inString.end(),
			inSubString.begin(), inSubString.end(),
			[&loc](char ch1, char ch2) { return std::tolower(ch1, loc) == std::tolower(ch2, loc); }
		);
		return (it != inString.end());
	}

	std::string trim(const std::string& s)
	{
		if (s.empty())
			return s;

		size_t first = s.find_first_not_of(' ');
		size_t last = s.find_last_not_of(' ');
		return s.substr(first, (last - first + 1));
	}

}