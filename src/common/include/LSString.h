#ifndef _LS_STRING_H_
#define _LS_STRING_H_

#include <string>
#include <cstdint>
#include <unordered_map>

class LSString
{
public:
	LSString();
	LSString(const std::string& s);

	virtual size_t Decode(const uint8_t* buffer, size_t size);
	virtual size_t Encode(uint8_t* buffer, size_t size);
	virtual std::string Serialise() const;
	virtual void Deserialise(const std::string& ss);
	virtual std::string GetHeaderRow() const;

	std::string Str() const
	{
		return m_str;
	}

	std::string& Str()
	{
		return m_str;
	}

protected:
	virtual size_t DecodeString(const uint8_t* string, size_t len);
	virtual size_t EncodeString(uint8_t* string, size_t len);
	std::string DecodeChar(uint8_t chr);
	size_t EncodeChar(std::string str, size_t index, uint8_t& chr);
	virtual const std::unordered_map<uint8_t, std::string>& Charmap() const;
	std::string m_str;
};

#endif // _LS_STRING_H_
