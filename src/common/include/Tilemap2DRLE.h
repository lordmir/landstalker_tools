#ifndef _TILEMAP_2D_RLE_
#define _TILEMAP_2D_RLE_

#include <vector>
#include <cstdint>
#include <cstdlib>
#include "Tile.h"

class Tilemap2D
{
public:
	Tilemap2D(size_t width, size_t height);
	Tilemap2D(const uint8_t* compressed_data, size_t size);
	std::vector<uint8_t> Compress() const;
	size_t GetHeight() const;
	size_t GetWidth() const;
	Tile GetTile(size_t x, size_t y) const;
	void SetTile(const Tile& tile, size_t x, size_t y);
	Tile* Data();
private:
	size_t m_width;
	size_t m_height;
	std::vector<Tile> m_tiles;
};

#endif // _TILEMAP_2D_RLE_
