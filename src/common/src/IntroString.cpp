#include "IntroString.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>

const std::unordered_map<uint8_t, std::string> INTRO_CHARSET = {
	{ 0, " "}, { 1, "A"}, { 2, "B"}, { 3, "C"}, { 4, "D"}, { 5, "E"}, { 6, "F"}, { 7, "G"},
	{ 8, "H"}, { 9, "I"}, {10, "J"}, {11, "K"}, {12, "L"}, {13, "M"}, {14, "N"}, {15, "O"},
	{16, "P"}, {17, "Q"}, {18, "R"}, {19, "S"}, {20, "T"}, {21, "U"}, {22, "V"}, {23, "W"},
	{24, "X"}, {25, "Y"}, {26, "Z"}, {27, "1"}, {28, "2"}, {29, "3"}
};

IntroString::IntroString()
	: LSString(),
	  m_line1Y(0),
	  m_line1X(0),
	  m_line2Y(0),
	  m_line2X(0),
	  m_displayTime(0)
{
}

IntroString::IntroString(uint16_t line1_y, uint16_t line1_x, uint16_t line2_y, uint16_t line2_x, uint16_t display_time, std::string str)
	: LSString(str),
	m_line1Y(line1_y),
	m_line1X(line1_x),
	m_line2Y(line2_y),
	m_line2X(line2_x),
	m_displayTime(display_time)
{
}

IntroString::IntroString(const std::string& serialised)
{
	Deserialise(serialised);
}

std::string IntroString::Serialise() const
{
	std::ostringstream ss;
	ss << m_line1X << "\t";
	ss << m_line1Y << "\t";
	ss << m_line2X << "\t";
	ss << m_line2Y << "\t";
	ss << m_displayTime << "\t";
	ss << m_str << "\t";
	ss << m_line2;
	return ss.str();
}

void IntroString::Deserialise(const std::string& in)
{
	std::istringstream liness(in);
	std::string cell;
	std::getline(liness, cell, '\t');
	m_line1X = std::atoi(cell.c_str());
	std::getline(liness, cell, '\t');
	m_line1Y = std::atoi(cell.c_str());
	std::getline(liness, cell, '\t');
	m_line2X = std::atoi(cell.c_str());
	std::getline(liness, cell, '\t');
	m_line2Y = std::atoi(cell.c_str());
	std::getline(liness, cell, '\t');
	m_displayTime = std::atoi(cell.c_str());
	std::getline(liness, m_str, '\t');
	std::getline(liness, m_line2, '\t');
}

std::string IntroString::GetHeaderRow() const
{
	return "Line1_X\tLine1_Y\tLine2_X\tLine2_Y\tDisplayTime\tLine1\tLine2";
}

template<class T>
inline T ReadNum(const uint8_t* buffer, size_t size)
{
	if (size < sizeof(T))
	{
		throw std::runtime_error("Not enough bytes in buffer to decode string params");
	}
	T retval = 0;
	for (size_t i = 0; i < sizeof(T); ++i)
	{
		retval <<= 8;
		retval |= buffer[i];
	}
	return retval;
}

template<class T>
inline size_t WriteNum(uint8_t* buffer, size_t size, T value)
{
	if (size < sizeof(uint16_t))
	{
		throw std::runtime_error("Not enough bytes in buffer to decode string params");
	}
	for (size_t i = 0; i < sizeof(T); ++i)
	{
		buffer[i] = (value >> (sizeof(T) - i - 1) * 8) & 0xFF;
	}
	return sizeof(T);
}

size_t IntroString::Decode(const uint8_t* buffer, size_t size)
{
	m_line1Y = ReadNum<uint16_t>(buffer, size);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	m_line1X = ReadNum<uint16_t>(buffer, size);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	m_line2Y = ReadNum<uint16_t>(buffer, size);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	m_line2X = ReadNum<uint16_t>(buffer, size);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	m_displayTime = ReadNum<uint16_t>(buffer, size);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	return sizeof(uint16_t) * 5 + DecodeString(buffer, size);
}

size_t IntroString::Encode(uint8_t* buffer, size_t size) const
{
	WriteNum<uint16_t>(buffer, size, m_line1Y);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	WriteNum<uint16_t>(buffer, size, m_line1X);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	WriteNum<uint16_t>(buffer, size, m_line2Y);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	WriteNum<uint16_t>(buffer, size, m_line2X);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	WriteNum<uint16_t>(buffer, size, m_displayTime);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	return sizeof(uint16_t) * 5 + EncodeString(buffer, size);
}

uint16_t IntroString::GetLine1Y() const
{
	return m_line1Y;
}

uint16_t IntroString::GetLine1X() const
{
	return m_line1X;
}

uint16_t IntroString::GetLine2Y() const
{
	return m_line2Y;
}

uint16_t IntroString::GetLine2X() const
{
	return m_line2X;
}

uint16_t IntroString::GetDisplayTime() const
{
	return m_displayTime;
}

size_t IntroString::DecodeString(const uint8_t* string, size_t len)
{
	m_str = "";
	size_t i = 0;
	while (string[i] != 0xFF)
	{
		if (i < 16)
		{
			m_str += DecodeChar(string[i++]);
		}
		else
		{
			m_line2 += DecodeChar(string[i++]);
		}
		if(i >= len)
		{
			throw std::runtime_error("Not enough bytes in buffer to decode string");
		}
	}
	return i + 1;
}

size_t IntroString::EncodeString(uint8_t* string, size_t len) const
{
	size_t i = 0;
	size_t j = 0;
	while (j < m_str.size() && i < len)
	{
		j += EncodeChar(m_str, j, string[i++]);
	}
	if (m_line2.empty() == false)
	{
		while (i < 16)
		{
			EncodeChar(" ", 0, string[i++]);
		}
		j = 0;
		while (j < m_line2.size() && i < len)
		{
			j += EncodeChar(m_line2, j, string[i++]);
		}
	}
	string[i] = 0xFF;
	return i + 1;
}

const std::unordered_map<uint8_t, std::string>& IntroString::Charmap() const
{
	return INTRO_CHARSET;
}
