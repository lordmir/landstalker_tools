#include "Tilemap3DCmp.h"

#include <stdexcept>
#include <unordered_map>
#include <set>
#include <map>
#include <functional>
#include <cstdint>
#include <algorithm>

#include "BitBarrel.h"
#include "BitBarrelWriter.h"

uint16_t getCodedNumber(BitBarrel& bb)
{
    uint16_t exp = 0, num = 0;
    
    while(!bb.getNextBit())
    {
        exp++;
    }
    
    if(exp)
    {
        num = 1 << exp;
        num += bb.readBits(exp);
    }
    
    return num;
}

uint16_t ilog2(uint16_t num)
{
    uint16_t ret = 0;
    while(num)
    {
        num >>= 1;
        ret++;
    }
    return ret;
}

uint16_t Tilemap3DCmp::Decode(const uint8_t* src, RoomTilemap& tilemap)
{
    BitBarrel bb(src);
    

    tilemap.left   = bb.readBits(8);
    tilemap.top    = bb.readBits(8);
    tilemap.width  = bb.readBits(8) + 1;
    tilemap.height = (bb.readBits(8) + 1) / 2;
    
    uint16_t tileDictionary[2] = {0, 0};
    uint16_t offsetDictionary[14] = {0xFFFF,
                                     1,
                                     2,
                                     static_cast<uint16_t>(tilemap.GetWidth()),
                                     static_cast<uint16_t>(tilemap.GetWidth() *2u),
                                     static_cast<uint16_t>(tilemap.GetWidth() + 1),
                                     0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t t = tilemap.GetWidth() * tilemap.GetHeight() * 2;
    std::vector<uint16_t> buffer(t,0);
    
    tileDictionary[1] = bb.readBits(10);
    tileDictionary[0] = bb.readBits(10);
    
    for(size_t i = 6; i < 14; ++i)
    {
        offsetDictionary[i] = bb.readBits(12);
    }
    
    int16_t dst_addr = -1;
    
    while(true)
    {
        uint16_t start = getCodedNumber(bb);
        
        if(!start) start++;
        dst_addr += start;
        
        if(dst_addr >= t)
        {
            break;
        }
        
        uint8_t command = bb.readBits(3);
        if(command > 5)
        {
            command = 6 + (((command & 1) << 2) | bb.readBits(2));
        }
        buffer[dst_addr] = offsetDictionary[command];
        
        if(bb.getNextBit())
        {
            uint16_t row_addr = dst_addr;
            bool width_offset = bb.getNextBit();
            do
            {
                do
                {
                    row_addr += tilemap.GetWidth() + (width_offset ? 1 : 0);
                    buffer[row_addr] = offsetDictionary[command];
                } while(bb.getNextBit());
                width_offset = !width_offset;
            } while(bb.getNextBit());
        }
    }
    
    uint16_t tiles[2] = {tileDictionary[0], tileDictionary[1]};
    dst_addr = 0;
    do
    {
        uint16_t operand = buffer[dst_addr];
        uint16_t offset;
        if(operand != 0xFFFF)
        {
            offset = dst_addr - operand;
            do
            {
                buffer[dst_addr++] = buffer[offset++];
            } while ((dst_addr < t) && (buffer[dst_addr] == 0));
        }
        else
        {
            do
            {
                operand = bb.readBits(2);
                uint16_t value = 0;
                switch(operand)
                {
                    case 0:
                        if(tiles[0])
                        {
                            value = bb.readBits(ilog2(tiles[0]));
                        }
                        buffer[dst_addr++] = value;
                        break;
                    case 1:
                        if(tiles[1] != tileDictionary[1])
                        {
                            value = bb.readBits(ilog2(tiles[1] - tileDictionary[1]));
                        }
                        value += tileDictionary[1];
                        buffer[dst_addr++] = value;
                        break;
                    case 2:
                        value = tiles[0]++;
                        buffer[dst_addr++] = value;
                        break;
                    case 3:
                        value = tiles[1]++;
                        buffer[dst_addr++] = value;
                        break;
                }
            } while ((dst_addr < t) && (buffer[dst_addr] == 0));
        }
    } while(dst_addr < t);
    
    tilemap.background.resize(tilemap.width * tilemap.height);
    tilemap.foreground.resize(tilemap.width * tilemap.height);
    std::copy(buffer.begin() + t/2, buffer.end(), tilemap.background.begin());
    std::copy(buffer.begin(), buffer.begin() + t / 2, tilemap.foreground.begin());
    
    bb.advanceNextByte();
    tilemap.hmwidth = bb.readBits(8);
    tilemap.hmheight = bb.readBits(8);
    
    uint16_t hm_pattern = 0;
    uint16_t hm_rle_count = 0;
    
    tilemap.heightmap.assign(tilemap.hmwidth * tilemap.hmheight, 0);
    dst_addr = 0;
    for(size_t y = 0; y <  tilemap.hmheight; y++)
    {
        for(size_t x = 0; x <  tilemap.hmwidth; x++)
        {
            if(!hm_rle_count--)
            {
                uint8_t read_count = 0;
                hm_rle_count = 0;
                hm_pattern = bb.readBits(16);
                do
                {
                    read_count = bb.readBits(8);
                    hm_rle_count += read_count;
                } while(read_count == 0xFF);
            }
            tilemap.heightmap[dst_addr++] = hm_pattern;
        }
    }
    return t;
}

void makeCodedNumber(uint16_t value, BitBarrelWriter& bb)
{
    uint16_t exp = ilog2(value) - 1;
    uint16_t i = exp;
    uint16_t num = value - (1 << exp);

    while (i--)
    {
        bb.WriteBits(0, 1);
    }
    bb.WriteBits(1, 1);

    if (exp)
    {
        bb.WriteBits(num, exp);
    }
}

int findMatchFrequency(const std::vector<uint16_t>& input, size_t offset, std::unordered_map<int, int>& fc)
{
    size_t lookback_size = std::min<size_t>(offset, 4095);
    size_t lookahead_size = input.size() - offset;
    int best = 0;
    for (size_t b = 1; b <= lookback_size; ++b)
    {
        int match_run = 0;
        for (size_t m = 0; m < lookahead_size; ++m)
        {
            if (input[offset - b + m] != input[offset + m])
            {
                break;
            }
            match_run++;
        }
        if (match_run > best)
        {
            best = match_run;
        }
    }
    if (best < 2) return 0;
    for (size_t b = 1; b <= lookback_size; ++b)
    {
        int match_run = 0;
        for (size_t m = 0; m < lookahead_size; ++m)
        {
            if (input[offset - b + m] != input[offset + m])
            {
                break;
            }
            match_run++;
        }
        if (match_run == best)
        {
            fc[b] += match_run;
        }
    }
    return best;
}

std::pair<int, int> findMatch(const std::vector<uint16_t>& input, size_t offset, const std::vector<uint16_t>& back_offsets)
{
    size_t lookback_size = std::min<size_t>(offset, 4095);
    size_t lookahead_size = input.size() - offset;
    std::pair<int, int> ret = { 0,0 };
    for (size_t i = 0; i < back_offsets.size(); ++i)
    {
        size_t b = back_offsets[i];
        if ((b == 0) || (b > lookback_size)) continue;
        int match_run = 0;
        for (size_t m = 0; m < lookahead_size; ++m)
        {
            if (input[offset - b + m] != input[offset + m])
            {
                break;
            }
            match_run++;
        }
        if (match_run > ret.second)
        {
            ret.second = match_run;
            ret.first = i;
        }
    }
    if (ret.second == 0)
    {
        ret.first = 0;
        ret.second = 1;
    }

    return ret;
}

uint16_t Tilemap3DCmp::Encode(const RoomTilemap& tilemap, uint8_t* dst, size_t size)
{
    BitBarrelWriter cmap;
    std::vector<uint16_t> offsets = { 0, 1, 2, static_cast<uint16_t>(tilemap.GetWidth()), static_cast<uint16_t>(tilemap.GetWidth() * 2), static_cast<uint16_t>(tilemap.GetWidth() + 1)};
    struct LZ77Entry
    {
        LZ77Entry(int run_length_in, int back_offset_idx_in, int index_in)
            : run_length(run_length_in), back_offset_idx(back_offset_idx_in), index(index_in)
        {}
        int run_length;
        int back_offset_idx;
        int index;
        std::vector<std::pair<bool, int>> vertical_info;
    };
    struct TileEntry
    {
        TileEntry(uint8_t code_in, uint16_t data_in, uint8_t data_length_in)
            : code(code_in), data(data_in), data_length(data_length_in)
        {}
        uint8_t code;
        uint16_t data;
        uint8_t data_length;
    };
    std::vector<LZ77Entry> lz77;
    uint16_t tile_dict[2] = { 0 };
    uint16_t tile_increment[2] = { 0 };
    std::vector<TileEntry> tile_entries;
    std::vector<std::pair<int, uint16_t>> heightmap;

    // COMPRESS MAP
    // Combine foreground and background
    std::vector<uint16_t> tiles(tilemap.GetWidth()* tilemap.GetHeight() * 2);
    std::copy(tilemap.foreground.begin(), tilemap.foreground.end(), tiles.begin());
    std::copy(tilemap.background.begin(), tilemap.background.end(), tiles.begin() + tiles.size()/2);
    // First stage of map compression involves LZ77 with a fixed-size dictionary
    // STEP 1: Run map through LZ77 compressor.
    std::unordered_map<int, int> offset_freq_count;
    std::vector<bool> compressed(tiles.size(), false);
    size_t idx = 1;
    do
    {
        int run = findMatchFrequency(tiles, idx, offset_freq_count);
        if (run == 0)
        {
            idx++;
        }
        else
        {
            idx += run;
        }
    } while (idx < tiles.size());

    // STEP 2: Identify top 8 back offsets and add to back offset dictionary

    typedef std::function<bool(const std::pair<int, int>&, const std::pair<int, int>&)> Comparator;
    Comparator comparator = [](const std::pair<int, int>& p1, const std::pair<int, int>& p2)
    {
        return p1.second > p2.second;
    };

    std::multiset<std::pair<int, int>, Comparator> frequency_counts(offset_freq_count.begin(), offset_freq_count.end(), comparator);
    for (auto it = frequency_counts.cbegin(); it != frequency_counts.cend(); ++it)
    {
        //std::cout << "Offset " << std::hex << it->first << ": " << std::dec << it->second << std::endl;
        if (std::find(offsets.begin(), offsets.end(), it->first) == offsets.end())
        {
            offsets.push_back(it->first);
        }
    }
    offsets.resize(14);

    for (size_t i = 0; i < offsets.size(); ++i)
    {
        //std::cout << "Offset " << std::hex << offsets[i] << ": " << std::dec << offset_freq_count[offsets[i]] << std::endl;
    }

    // STEP 3: Compress map using LZ77 and the back offset dictionary created during step 2

    lz77.emplace_back(1, 0, 0);
    idx = 1;
    size_t run_count = 0;
    do
    {
        auto result = findMatch(tiles, idx, offsets);
        if ((result.first != 0) || (lz77.back().back_offset_idx != 0))
        {
            lz77.emplace_back(result.second, result.first, idx);
            run_count = 0;
        }
        else
        {
            lz77.back().run_length++;
        }
        if (lz77.back().back_offset_idx == 0)
        {
            //std::cout << " LOAD TILE @ " << idx << std::endl;
            compressed[idx] = false;
            idx++;
        }
        else
        {
            //std::cout << " LZ77 @ " << idx << " : copy " << lz77.back().run_length
            //          << " bytes from offset " << lz77.back().back_offset_idx << "("
            //          << (idx - offsets[lz77.back().back_offset_idx]) << ")" << std::endl;
            std::fill(compressed.begin() + idx, compressed.begin() + idx + lz77.back().run_length, true);
            idx += lz77.back().run_length;
        }
    } while (idx < tiles.size());

    // STEP 5: For each LZ77 operation, scan downwards and down-right in 2D, and combine entries
    for (auto it = lz77.begin(); it != lz77.end(); ++it)
    {
        size_t count = 0;
        bool right = false;
        bool begin = true;
        if (it->back_offset_idx != -1)
        {
            size_t next = it->index;
            size_t prev = next;
            while (next < tiles.size())
            {
                next += tilemap.GetWidth() + (right ? 1 : 0);
                auto nit = std::find_if(it, lz77.end(), [&](const LZ77Entry& comp)
                    {
                        return (comp.index == next) && (comp.back_offset_idx == it->back_offset_idx);
                    });
                if (nit != lz77.end())
                {
                    count++;
                    nit->back_offset_idx = -1;
                    prev = next;
                }
                else
                {
                    if (count > 0)
                    {
                        it->vertical_info.emplace_back(right, count);
                        count = 0;
                    }
                    else
                    {
                        if (begin == false)
                        {
                            break;
                        }
                    }
                    begin = false;
                    right = !right;
                    next = prev;
                }
            }
        }
    }

    auto it = std::remove_if(lz77.begin(), lz77.end(), [](const LZ77Entry& comp)
        {
            return comp.back_offset_idx == -1;
        });
    lz77.erase(it, lz77.end());

    // STEP 6: Identify sequential runs of tiles, the longest run including decrements will be
    //         stored in element 1 of the tile dict. The longest run where ilog2(base) == ilog2(highest tile #)
    //         will be stored in element 2 of the tile dict.

    std::map<uint16_t, int> incrementing_tile_counts;
    std::map<uint16_t, int> ranged_tile_counts;
    for (size_t i = 0; i < tiles.size(); ++i)
    {
        if (compressed[i] == false)
        {
            for (auto& itc : incrementing_tile_counts)
            {
                if (tiles[i] == itc.first + itc.second)
                {
                    itc.second++;
                }
                if ((tiles[i] >= itc.first) && (tiles[i] < itc.first + itc.second))
                {
                    ranged_tile_counts[tiles[i]]++;
                }
            }
            if (incrementing_tile_counts.find(tiles[i]) == incrementing_tile_counts.end())
            {
                incrementing_tile_counts[tiles[i]] = 1;
            }
        }
    }
    std::multiset<std::pair<uint16_t, int>, Comparator> incrementing_tile_freqs(
        incrementing_tile_counts.begin(), incrementing_tile_counts.end(), comparator);
    uint16_t max_tile = incrementing_tile_counts.rbegin()->first;
    uint16_t min_dict_entry = 1 << (ilog2(max_tile) - 1);
    tile_dict[1] = 0;
    for (auto& irt : incrementing_tile_counts)
    {
        if ((tile_dict[1] == 0) && (irt.first >= min_dict_entry))
        {
            tile_dict[1] = irt.first;
            irt.second = 0;
        }
        else
        {
            irt.second *= 4;
            irt.second += ranged_tile_counts[irt.first];
        }
    }
    if (tile_dict[1] == 0)
    {
        tile_dict[1] = min_dict_entry;
    }
    std::multiset<std::pair<uint16_t, int>, Comparator> scored_tile_freqs(
        incrementing_tile_counts.begin(), incrementing_tile_counts.end(), comparator);

    tile_dict[0] = incrementing_tile_freqs.begin()->first;

    // STEP 7: Start to compress tile data. Identify if tile is (1) equal to any in tile dictionary + increment,
    //         (2) between tileDict[0] and tileDict[0] + tileDictIncr[0], or (3) none of the above.

    size_t z = 0; // LZ77 entry index
    bool lz77_mode = false;
    for (size_t i = 0; i < tiles.size(); ++i)
    {
        if (compressed[i] == false)
        {
            if (tiles[i] == tile_dict[0] + tile_increment[0])
            {
                tile_increment[0]++;
                //std::cout << "INCREMENT TILE 1 [" << std::hex << rt.tiles[i] << " @ " << std::dec << i << std::endl;
                tile_entries.emplace_back(3, 0, 0);
            }
            else if (tiles[i] == tile_dict[1] + tile_increment[1])
            {
                tile_increment[1]++;
                //std::cout << "INCREMENT TILE 2 [" << std::hex << rt.tiles[i] << " @ " << std::dec << i << std::endl;
                tile_entries.emplace_back(2, 0, 0);
            }
            else if ((tiles[i] >= tile_dict[0]) && (tiles[i] < (tile_dict[0] + tile_increment[0])))
            {
                //std::cout << "PLACE REL TILE [" << std::hex << rt.tiles[i] << " @ " << std::dec << i << std::endl;
                tile_entries.emplace_back(1, static_cast<int>(tiles[i] - tile_dict[0]), static_cast<uint16_t>(ilog2(tile_increment[0])));
            }
            else
            {
                //std::cout << "PLACE TILE " << std::hex << rt.tiles[i] << " @ " << std::dec << i << std::endl;
                tile_entries.emplace_back(0, tiles[i], ilog2(tile_dict[1]));
            }
        }
    }

    // STEP 8: Map compression complete, compress heightmap. For heightmap data, compress as RLE.

    for (size_t i = 0; i < tilemap.heightmap.size(); ++i)
    {
        if (heightmap.empty() || heightmap.back().second != tilemap.heightmap[i])
        {
            heightmap.emplace_back(0, tilemap.heightmap[i]);
        }
        else
        {
            heightmap.back().first++;
        }
    }

    // STEP 9: Compression complete! Begin output:

    // Top, Left, Width, Height
    cmap.Write<uint8_t>(tilemap.GetLeft());
    cmap.Write<uint8_t>(tilemap.GetTop());
    cmap.Write<uint8_t>(tilemap.GetWidth() - 1);
    cmap.Write<uint8_t>(tilemap.GetHeight() * 2 - 1);
    // Tile Dictionary
    cmap.WriteBits(tile_dict[0], 10);
    cmap.WriteBits(tile_dict[1], 10);
    // Offset Dictionary
    for (size_t i = 0; i < 8; ++i)
    {
        cmap.WriteBits(offsets[6 + i], 12);
    }
    // LZ77 Data: run length, back offset index, vertical run data
    size_t last_idx = -1;
    size_t prev_run = 1;
    for (const auto& entry : lz77)
    {
        // Run length
        makeCodedNumber(static_cast<uint16_t>(entry.index - last_idx), cmap);
        last_idx = entry.index;
        prev_run = entry.run_length;
        // Back index
        if (entry.back_offset_idx < 6)
        {
            cmap.WriteBits(entry.back_offset_idx, 3);
        }
        else
        {
            cmap.WriteBits(3, 2);
            cmap.WriteBits(entry.back_offset_idx - 6, 3);
        }
        //Vertical run
        if (!entry.vertical_info.empty())
        {
            cmap.WriteBits(1, 1);

            bool begin = true;
            for (const auto& v : entry.vertical_info)
            {
                if (begin)
                {
                    cmap.WriteBits(entry.vertical_info[0].first, 1);
                    begin = false;
                }
                else
                {
                    cmap.WriteBits(1, 1);
                }
                for (int i = 1; i < v.second; ++i)
                {
                    cmap.WriteBits(1, 1);
                }
                cmap.WriteBits(0, 1);
            }
            cmap.WriteBits(0, 1);
        }
        else
        {
            cmap.WriteBits(0, 1);
        }
    }
    if (last_idx < tiles.size())
    {
        makeCodedNumber(static_cast<uint16_t>(tiles.size() - last_idx + 1), cmap);
    }
    else
    {
        makeCodedNumber(1, cmap);
    }

    // Tile data: 2-bit operand + variable length data

    for (const auto& entry : tile_entries)
    {
        cmap.WriteBits(entry.code, 2);
        switch (entry.code)
        {
        case 0:
        case 1:
            cmap.WriteBits(entry.data, entry.data_length);
            break;
        default:
            break;
        }
    }
    // Advance to next byte
    cmap.AdvanceNextByte();
    // Heightmap dims
    cmap.Write<uint8_t>(tilemap.hmwidth);
    cmap.Write<uint8_t>(tilemap.hmheight);
    // Heightmap data: pattern + run length. If run length >= 0xFF, output 0xFF then run length - 0xFF
    for (const auto& entry : heightmap)
    {
        cmap.Write<uint16_t>(entry.second);
        int len = entry.first;
        while (len >= 0xFF)
        {
            cmap.Write<uint8_t>(0xFF);
            len -= 0xFF;
        }
        cmap.Write<uint8_t>(len);
    }
    if (cmap.GetByteCount() <= size)
    {
        std::copy(cmap.Begin(), cmap.End(), dst);
    }
    else
    {
        throw std::runtime_error("Output buffer not large enough to hold result.");
    }
    return static_cast<uint16_t>(cmap.GetByteCount());
}