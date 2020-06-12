#include <LSString.h>

#include <stdexcept>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <iostream>

const std::unordered_map<uint8_t, std::string> CHARSET = {
	{ 0, " "}, { 1, "0"}, { 2, "1"}, { 3, "2"}, { 4, "3"}, { 5, "4"}, { 6, "5"}, { 7, "6"},
	{ 8, "7"}, { 9, "8"}, {10, "9"}, {11, "A"}, {12, "B"}, {13, "C"}, {14, "D"}, {15, "E"},
	{16, "F"}, {17, "G"}, {18, "H"}, {19, "I"}, {20, "J"}, {21, "K"}, {22, "L"}, {23, "M"},
	{24, "N"}, {25, "O"}, {26, "P"}, {27, "Q"}, {28, "R"}, {29, "S"}, {30, "T"}, {31, "U"},
	{32, "V"}, {33, "W"}, {34, "X"}, {35, "Y"}, {36, "Z"}, {37, "a"}, {38, "b"}, {39, "c"},
	{40, "d"}, {41, "e"}, {42, "f"}, {43, "g"}, {44, "h"}, {45, "i"}, {46, "j"}, {47, "k"},
	{48, "l"}, {49, "m"}, {50, "n"}, {51, "o"}, {52, "p"}, {53, "q"}, {54, "r"}, {55, "s"},
	{56, "t"}, {57, "u"}, {58, "v"}, {59, "w"}, {60, "x"}, {61, "y"}, {62, "z"}, {63, "*"},
	{64, "."}, {65, ","}, {66, "?"}, {67, "!"}, {68, "/"}, {69, "<"}, {70, ">"}, {71, ":"},
	{72, "-"}, {73, "\'"}, {74, "\""}, {75, "%"}, {76, "#"}, {77, "&"}, {78, "("}, {79, ")"},
	{80, "="}, {81, "{NW}"}, {82, "{NE}"}, {83, "{SE}"}, {84, "{SW}"}
};

LSString::LSString()
{
}

LSString::LSString(const std::string& s)
	: m_str(s)
{
}

size_t LSString::Decode(const uint8_t* buffer, size_t size)
{
	return DecodeString(buffer, size);
}

size_t LSString::Encode(uint8_t* buffer, size_t size)
{
	return EncodeString(buffer, size);
}

std::string LSString::Serialise() const
{
	std::ostringstream ss;
	ss << m_str;
	return ss.str();
}

void LSString::Deserialise(const std::string& in)
{
	m_str = in;
}

std::string LSString::GetHeaderRow() const
{
	return "String";
}

size_t LSString::DecodeString(const uint8_t* string, size_t len)
{
	m_str = "";
	size_t strsize = string[0];
	const uint8_t* c = string + 1;
	if (strsize > len)
	{
		throw std::runtime_error("Not enough bytes in buffer to decode string");
	}
	while (strsize > 0)
	{
		m_str += DecodeChar(*c++);
		strsize--;
	}
	return c - string;
}

size_t LSString::EncodeString(uint8_t* string, size_t len)
{
	size_t i = 1;
	size_t j = 0;
	while (j < m_str.size() && i < len)
	{
		j += EncodeChar(m_str, j, string[i++]);
	}
	if (i > 0x100)
	{
		throw std::runtime_error("String is too long.");
	}
	string[0] = (i - 1) & 0xFF;
	return i;
}

std::string LSString::DecodeChar(uint8_t chr)
{
	if (Charmap().find(chr) != Charmap().end())
	{
		return Charmap().at(chr);
	}
	else
	{
		std::ostringstream ss;
		ss << "{" << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
		   << static_cast<unsigned>(chr) << "}";
		return ss.str();
	}
}

size_t LSString::EncodeChar(std::string str, size_t index, uint8_t& chr)
{
	for (const auto& c : Charmap())
	{
		if (c.second.size() <= str.size() - index)
		{
			if (c.second == str.substr(index, c.second.size()))
			{
				chr = c.first;
				return c.second.size();
			}
		}
		
	}
	if (str[index] == '{')
	{
		size_t end_of_number = str.find_first_of('}', index + 1);
		if (end_of_number == std::string::npos)
		{
			std::ostringstream ss;
			ss << "Bad character number in position " << index << " found when parsing string.";
			throw std::runtime_error(ss.str());
		}
		std::string charnum = str.substr(index + 1, end_of_number - index - 1);
		std::istringstream iss(charnum);
		size_t num;
		iss >> std::hex >> num;
		if (num > 0xFF)
		{
			std::ostringstream ss;
			ss << "Bad character number in position " << index << " found when parsing string.";
			throw std::runtime_error(ss.str());
		}
		chr = static_cast<uint8_t>(num);
		return 2 + charnum.length();
	}
	std::ostringstream ss;
	ss << "Bad character \'" << str[index] << "\' in position " << index << " found when parsing string.";
	throw std::runtime_error(ss.str());
	chr = 0xFF;
	return 0;
}

const std::unordered_map<uint8_t, std::string>& LSString::Charmap() const
{
	return CHARSET;
}
