#ifndef TILEATTRIBUTES_H
#define TILEATTRIBUTES_H

#include <cstdint>

class TileAttributes
{
public:
    TileAttributes();
    TileAttributes(bool hflip, bool vflip, bool priority, uint8_t pal = 0);
    
    enum Attribute {ATTR_HFLIP, ATTR_VFLIP, ATTR_PRIORITY};

    void SetAttribute(const Attribute& attr);
    void ClearAttribute(const Attribute& attr);
    bool GetAttribute(const Attribute& attr) const;

    uint8_t GetPallette() const;
    void SetPalette(uint8_t palette);
    
private:
    bool m_hflip;
    bool m_vflip;
    bool m_priority;
    uint8_t m_pal;
};

#endif // TILEATTRIBUTES_H
