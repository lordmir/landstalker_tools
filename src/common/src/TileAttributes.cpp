#include "TileAttributes.h"

#include <stdexcept>

TileAttributes::TileAttributes()
: m_hflip(false),
  m_vflip(false),
  m_priority(false),
  m_pal(0)
{
}

TileAttributes::TileAttributes(bool hflip, bool vflip, bool priority, uint8_t pal)
: m_hflip(hflip),
  m_vflip(vflip),
  m_priority(priority),
  m_pal(pal)
{
}

void TileAttributes::SetAttribute(const TileAttributes::Attribute& attr)
{
    switch(attr)
    {
        case TileAttributes::ATTR_HFLIP:
            m_hflip = true;
            break;
        case TileAttributes::ATTR_VFLIP:
            m_vflip = true;
            break;
        case TileAttributes::ATTR_PRIORITY:
            m_priority = true;
            break;
        default:
            throw(std::runtime_error("Bad Attribute Type!"));
    }
}

void TileAttributes::ClearAttribute(const TileAttributes::Attribute& attr)
{
    switch(attr)
    {
        case TileAttributes::ATTR_HFLIP:
            m_hflip = false;
            break;
        case TileAttributes::ATTR_VFLIP:
            m_vflip = false;
            break;
        case TileAttributes::ATTR_PRIORITY:
            m_priority = false;
            break;
        default:
            throw(std::runtime_error("Bad Attribute Type!"));
    }
}

bool TileAttributes::GetAttribute(const TileAttributes::Attribute& attr) const
{
    bool retval = false;
    switch(attr)
    {
        case TileAttributes::ATTR_HFLIP:
            retval = m_hflip;
            break;
        case TileAttributes::ATTR_VFLIP:
            retval = m_vflip;
            break;
        case TileAttributes::ATTR_PRIORITY:
            retval = m_priority;
            break;
        default:
            throw(std::runtime_error("Bad Attribute Type!"));
    }
    return retval;
}

uint8_t TileAttributes::GetPallette() const
{
    return m_pal;
}

void TileAttributes::SetPalette(uint8_t palette)
{
    if (palette > 3)
    {
        throw(std::runtime_error("Bad palette ID!"));
    }
    else
    {
        m_pal = palette;
    }
}
