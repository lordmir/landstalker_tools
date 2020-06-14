#include "EndCreditString.h"
#include <stdexcept>
#include <sstream>

const std::unordered_map<uint8_t, std::string> ENDING_CHARSET = {
	{ 1, " "}, { 2, "A"}, { 3, "B"}, { 4, "C"}, { 5, "D"}, { 6, "E"}, { 7, "F"}, { 8, "G"},
	{ 9, "H"}, {10, "I"}, {11, "J"}, {12, "K"}, {13, "L"}, {14, "M"}, {15, "N"}, {16, "O"},
	{17, "P"}, {18, "Q"}, {19, "R"}, {20, "S"}, {21, "T"}, {22, "U"}, {23, "V"}, {24, "W"},
	{25, "X"}, {26, "Y"}, {27, "Z"}, {28, "a"}, {29, "b"}, {30, "c"}, {31, "d"}, {32, "e"},
	{33, "f"}, {34, "g"}, {35, "h"}, {36, "i"}, {37, "j"}, {38, "k"}, {39, "l"}, {40, "m"},
	{41, "n"}, {42, "o"}, {43, "p"}, {44, "q"}, {45, "r"}, {46, "s"}, {47, "t"}, {48, "u"},
	{49, "v"}, {50, "w"}, {51, "x"}, {52, "y"}, {53, "z"}, {54, "1"}, {55, "3"}, {56, "9"},
	{57, "(C)"}, {58, "(3)"}, {59, "-"}, {60, ","}, {61, "."},
	{64, "{K1}"}, {65, "{K2}"}, {66, "{K3}"}, {67, "{K4}"},
	{128, "_"}, {129, "{UL1}" }, { 130, "{UL2}" },
	{ 131, "{SEGA_LOGO}" }, {132, "{CLIMAX_LOGO}"}, {133, "{DDS520_LOGO}"}, {134, "{MIRAGE_LOGO}"}
};

EndCreditString::EndCreditString()
	: LSString(),
	m_height(0),
	m_column(-1)
{
}

EndCreditString::EndCreditString(int8_t fmt0, int8_t fmt1, const std::string& str)
	: LSString(str),
	m_height(fmt0),
	m_column(fmt1)
{
}

EndCreditString::EndCreditString(const std::string& serialised)
{
	Deserialise(serialised);
}

size_t EndCreditString::Decode(const uint8_t* buffer, size_t size)
{
	size_t ret = 0;
	if (size < 2)
	{
		throw std::runtime_error("Not enough bytes to decode string");
	}
	m_height = *reinterpret_cast<const int8_t*>(buffer++);
	m_column = *reinterpret_cast<const int8_t*>(buffer++);
	ret += 2;
	size -= 2;
	if (m_height != -1)
	{
		if (m_column < 0)
		{
			ret += DecodeString(buffer, size);
		}
		else
		{
			ret++;
			size--;
			m_str = DecodeChar(*buffer++);
		}
	}
	return ret;
}

size_t EndCreditString::Encode(uint8_t* buffer, size_t size) const
{
	size_t ret = 0;
	if (size < 2)
	{
		throw std::runtime_error("Not enough bytes to encode string");
	}
	*buffer++ = *reinterpret_cast<const uint8_t*>(&m_height);
	*buffer++ = *reinterpret_cast<const uint8_t*>(&m_column);
	size -= 2;
	ret += 2;
	if (m_height != -1)
	{
		if (m_column < 0)
		{
			ret += EncodeString(buffer, size);
		}
		else
		{
			ret++;
			EncodeChar(m_str, 0, *buffer++);
		}
	}
	return ret;
}

std::string EndCreditString::Serialise() const
{
	std::ostringstream ss;
	ss << static_cast<int>(m_height) << "\t";
	ss << -static_cast<int>(m_column) << "\t";
	ss << m_str;
	return ss.str();
}

void EndCreditString::Deserialise(const std::string& in)
{
	std::istringstream liness(in);
	std::string cell;
	std::getline(liness, cell, '\t');
	m_height = std::atoi(cell.c_str());
	std::getline(liness, cell, '\t');
	m_column = -std::atoi(cell.c_str());
	std::getline(liness, m_str, '\t');
}

std::string EndCreditString::GetHeaderRow() const
{
	return "Height\tColumn\tString";
}

uint8_t EndCreditString::GetHeight() const
{
	return m_height;
}

uint8_t EndCreditString::GetColumn() const
{
	return m_column;
}

size_t EndCreditString::DecodeString(const uint8_t* string, size_t len)
{
	m_str = "";
	const uint8_t* c = string;
	while (*c != 0x00)
	{
		m_str += DecodeChar(*c++);
		if (static_cast<size_t>(c - string) >= len)
		{
			throw std::runtime_error("Not enough bytes in buffer to decode string");
		}
	}
	return c - string + 1;
}

size_t EndCreditString::EncodeString(uint8_t* string, size_t len) const
{
	size_t i = 0;
	size_t j = 0;
	while (j < m_str.size() && i < len - 1)
	{
		j += EncodeChar(m_str, j, string[i++]);
	}
	string[i] = 0x00;
	return i + 1;
}

const std::unordered_map<uint8_t, std::string>& EndCreditString::Charmap() const
{
	return ENDING_CHARSET;
}
