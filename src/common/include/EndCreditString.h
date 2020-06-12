#ifndef _END_CREDIT_STRING_H_
#define _END_CREDIT_STRING_H_

#include <LSString.h>
#include <vector>

class EndCreditString : public LSString
{
public:
	EndCreditString();
	EndCreditString(int8_t fmt0, int8_t fmt1, const std::string& str);
	EndCreditString(int8_t fmt0, int8_t fmt1, const std::vector<uint8_t>& gfxParams, const std::string& str);
	EndCreditString(const std::string& serialised);

	virtual size_t Decode(const uint8_t* buffer, size_t size);
	virtual size_t Encode(uint8_t* buffer, size_t size);
	virtual std::string Serialise() const;
	virtual void Deserialise(const std::string& ss);
	virtual std::string GetHeaderRow() const;

	uint8_t GetHeight() const;
	uint8_t GetColumn() const;
	const std::vector<uint8_t>& GetGfxParams() const;
protected:
	virtual size_t DecodeString(const uint8_t* string, size_t len);
	virtual size_t EncodeString(uint8_t* string, size_t len);
	virtual const std::unordered_map<uint8_t, std::string>& Charmap() const;
private:
	int8_t m_height;
	int8_t m_column;
	std::vector<uint8_t> m_gfx_params;
};

#endif // _END_CREDIT_STRING_H