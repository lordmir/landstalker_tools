#ifndef LSTILEMAPCMP_H
#define LSTILEMAPCMP_H

#include <cstdint>
#include <vector>

struct RoomTilemap
{
    RoomTilemap()
    : foreground(),
      background(),
      heightmap(),
      left(0), top(0), width(0), height(0), hmwidth(0), hmheight(0)
    {
    }

    uint8_t GetLeft() const { return left; }
    uint8_t GetTop() const { return top; }
    uint8_t GetWidth() const { return width; }
    uint8_t GetHeight() const { return height; }

    std::vector<uint16_t> foreground;
    std::vector<uint16_t> background;
    std::vector<uint16_t> heightmap;
    uint8_t hmwidth;
    uint8_t hmheight;
    uint8_t left;
    uint8_t top;
    uint8_t width;
    uint8_t height;
};

class Tilemap3DCmp
{
public:
    static uint16_t Decode(const uint8_t* src, RoomTilemap& tilemap);
    static uint16_t Encode(const RoomTilemap& tilemap, uint8_t* dst, size_t size);
private:
    Tilemap3DCmp();
};

#endif // LSTILEMAPCMP_H
