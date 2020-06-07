#include <iostream>
#include <string>
#include <cstdint>
#include <exception>
#include <fstream>
#include <sstream>
#include <vector>
#include <iterator>

#include <sys/stat.h>

#define TCLAP_SETBASE_ZERO 1
#include <tclap/CmdLine.h>
#include <rapidcsv.h>
#include <LZ77.h>
#include <Tilemap2DRLE.h>
#include <Tile.h>
#include <Block.h>
#include <BlocksetCmp.h>

int main(int argc, char** argv)
{
	try
	{
		TCLAP::CmdLine cmd("Utility for converting 2D tilemaps and blocksets between binary, compressed binary and CSV.\n"
			"Part of the landstalker_tools set: github.com/lordmir/landstalker_tools\n"
			" - Written by LordMir, June 2020",
			' ', "0.1");

		std::vector<std::string> formats{"csv","map","lz77","rle","cbs"};
		TCLAP::ValuesConstraint<std::string> allowedVals(formats);

		TCLAP::UnlabeledValueArg<std::string> fileIn("input_file", "The input file (.map/.rle/.lz77/.csv/.cbs)", true, "", "in_filename");
		TCLAP::UnlabeledValueArg<std::string> fileOut("output_file", "The output file (.map/.rle/.lz77/.csv/.cbs)", true, "", "out_filename");
		TCLAP::ValueArg<std::string> inputFormat("i", "input_format", "Input format", true, "map", &allowedVals);
		TCLAP::ValueArg<std::string> outputFormat("o", "output_format", "Output format", true, "csv", &allowedVals);
		TCLAP::ValueArg<size_t> widthIn("w", "width", "Width of the 2D map in 8x8 tiles", false, 0, "width_tiles");
		TCLAP::ValueArg<size_t> heightIn("", "height", "Height of the 2D map in 8x8 tiles", false, 0, "height_tiles");
		TCLAP::SwitchArg force("f", "force", "Force overwrite if file already exists and no offset has been set", false);
		TCLAP::ValueArg<uint32_t> inOffset("", "inoffset", "Offset into the input file to start reading data, useful if working with the raw ROM", false, 0, "offset");
		TCLAP::ValueArg<uint32_t> outOffset("", "outoffset", "Offset into the output file to start writing data, useful if working with the raw ROM.\n"
			"**WARNING** This program will not make any attempt to rearrange data in the ROM. If the compressed "
			"size is greater than expected, then data could be overwritten!", false, 0, "offset");
		cmd.add(force);
		cmd.add(fileIn);
		cmd.add(fileOut);
		cmd.add(inputFormat);
		cmd.add(outputFormat);
		cmd.add(widthIn);
		cmd.add(heightIn);
		cmd.add(inOffset);
		cmd.add(outOffset);
		cmd.parse(argc, argv);

		std::vector<uint8_t> input;
		std::vector<uint8_t> output;
		size_t width = 0;
		size_t height = 0;
		size_t size = 0;
		if (inputFormat.getValue() == "map" || inputFormat.getValue() == "lz77")
		{
			if (widthIn.getValue() <= 0 && inOffset.isSet() == false)
			{
				throw std::runtime_error("Error: a valid width must be specified when converting binary or LZ77 compressed maps.");
			}
			if ((widthIn.getValue() <= 0 || heightIn.getValue()) && inOffset.isSet() == true)
			{
				throw std::runtime_error("Error: a valid width and height must be specified when reading binary or LZ77 compressed maps from ROM.");
			}
			width = widthIn.getValue();
			height = heightIn.getValue();
			size = width * height * 2;
		}
		else
		{
			if (widthIn.getValue() != 0 || heightIn.getValue() != 0)
			{
				std::cerr << "Warning: Width and height parameters will be ignored, as " << inputFormat.getValue()
					      << " format maps already contain this information." << std::endl;
			}
			if (inputFormat.getValue() == "cbs")
			{
				// Blocksets are essentially a list of four tiles. Each four tiles makes up a single 2x2 block
				width = 4;
				height = 0;
			}
		}

		// First, cache our input file
		std::ifstream ifs(fileIn.getValue(), std::ios::binary);
		if (ifs.good() == false)
		{
			std::ostringstream msg;
			msg << "Unable to open file \"" << fileIn.getValue() << "\" for reading.";
			throw std::runtime_error(msg.str());
		}
		else
		{
			ifs.unsetf(std::ios::skipws);

			ifs.seekg(0, std::ios::end);
			std::streampos filesize = ifs.tellg();
			ifs.seekg(0, std::ios::beg);

			if (filesize > 0 && inOffset.getValue() >= filesize)
			{
				std::ostringstream msg;
				msg << "Provided offset " << inOffset.getValue() << " is greater than the size of the file \"" << fileIn.getValue() << "\" (" << filesize << " bytes).";
				throw std::runtime_error(msg.str());
			}
			if (static_cast<int>(size) > (static_cast<int>(filesize) - inOffset.getValue()))
			{
				std::ostringstream msg;
				msg << "Expected map size (" << size << " bytes) is greater than the available size of the file \"" << fileIn.getValue() << "\" (" << (static_cast<int>(filesize) - inOffset.getValue()) << " bytes).";
				throw std::runtime_error(msg.str());
			}

			input.reserve(static_cast<unsigned int>(filesize));
			input.insert(input.begin(), std::istream_iterator<uint8_t>(ifs), std::istream_iterator<uint8_t>());
		}
		ifs.close();

		// Next, test our output file
		std::ifstream outfile_in(fileOut.getValue(), std::ios::binary);
		if (outfile_in.good() == false)
		{
			if (outOffset.isSet() == true)
			{
				std::ostringstream msg;
				msg << "Unable to write to offset as file \"" << fileOut.getValue() << "\" can't be opened.";
				throw std::runtime_error(msg.str());
			}
		}
		else
		{
			// Writing to an offset? Cache our output file
			if (outOffset.isSet() == true)
			{
				outfile_in.unsetf(std::ios::skipws);

				outfile_in.seekg(0, std::ios::end);
				std::streampos filesize = outfile_in.tellg();
				outfile_in.seekg(0, std::ios::beg);

				output.reserve(static_cast<unsigned int>(filesize));
				output.insert(output.begin(), std::istream_iterator<uint8_t>(outfile_in), std::istream_iterator<uint8_t>());
			}
			else if (force.isSet() == false)
			{
				std::ostringstream msg;
				msg << "Unable to write to file \"" << fileOut.getValue() << "\" as it already exists. Try running the command again with the -f flag.";
				throw std::runtime_error(msg.str());
			}
		}
		outfile_in.close();

		std::vector<Tile> map;
		std::vector<uint8_t> outbuffer;
		// Next, the conversion. Convert input to intermeditate binary
		if (inputFormat.getValue() == "map")
		{
			// If height not specified, calculate it based on the file size
			if (width > 0 && height == 0)
			{
				height = (input.size() - inOffset.getValue() + width * 2 - 1) / (width * 2);
			}
			if ((input.size() - inOffset.getValue()) % (width * 2) > 0)
			{
				std::cerr << "Warning: file size (" << (input.size() - inOffset.getValue()) << " bytes) is not an exact multiple of the map width. Excess bytes will be ignored." << std::endl;
				input.resize(inOffset.getValue() + width * height * 2);
			}
			map.reserve(width * height);
			auto it = input.begin() + inOffset.getValue();
			for (size_t i = 0; i < (width * height); ++i)
			{
				uint16_t t = (*it++ << 8);
				t |= *it++;
				map.push_back(t);
			}
		}
		else if (inputFormat.getValue() == "lz77")
		{
			std::vector<uint8_t> decomp(65536);
			size_t esize = 0;
			size_t dsize = LZ77::Decode(input.data() + inOffset.getValue(), input.size() - inOffset.getValue(), decomp.data(), esize);
			// If height not specified, calculate it based on the decompressed size
			if (width > 0 && height == 0)
			{
				height = (dsize + width * 2 - 1) / (width * 2);
			}
			decomp.resize(width*height*2);
			map.reserve(width* height);
			auto it = decomp.begin();
			for (size_t i = 0; i < (width * height); ++i)
			{
				uint16_t t = (*it++ << 8);
				t |= *it++;
				map.push_back(t);
			}
		}
		else if (inputFormat.getValue() == "rle")
		{
			Tilemap2D map2d(input.data() + inOffset.getValue(), input.size() - inOffset.getValue());
			width = map2d.GetWidth();
			height = map2d.GetHeight();
			map.reserve(width* height);
			for (size_t y = 0; y < height; ++y)
			{
				for (size_t x = 0; x < width; ++x)
				{
					map.push_back(map2d.GetTile(x, y));
				}
			}
		}
		else if (inputFormat.getValue() == "cbs")
		{
			// Compressed blockset
			std::vector<Block> blocks;
			height = BlocksetCmp::Decode(input.data() + inOffset.getValue(), input.size() - inOffset.getValue(), blocks);
			width = 4;
			map.reserve(width * height);
			for (const auto& block : blocks)
			{
				for (size_t x = 0; x < width; ++x)
				{
					map.push_back(block.GetTile(x));
				}
			}
		}
		else if (inputFormat.getValue() == "csv")
		{
			if (input.begin() + inOffset.getValue() == input.end())
			{
				// Special case - zero byte file.
				width = 0;
				height = 0;
			}
			else
			{
				std::stringstream ss(std::string(input.begin() + inOffset.getValue(), input.end()));
				rapidcsv::Document csv(ss, rapidcsv::LabelParams(-1, -1));
				// For CSV, first work out the number of rows and columns
				width = csv.GetColumnCount();
				height = csv.GetRowCount();
				if (width <= 0 || height <= 0)
				{
					throw std::runtime_error("Error: CSV malformed");
				}
				map.reserve(width * height);
				for (size_t y = 0; y < height; y++)
				{
					for (size_t x = 0; x < width; x++)
					{
						map.push_back(csv.GetCell<unsigned>(x, y));
					}
				}
			}
		}
		else
		{
			throw std::runtime_error("Unexpected input file format");
		}
		
		// Convert intermediate binary to output
		std::cout << "Writing " << width << "x" << height << " tilemap" << std::endl;
		if (outputFormat.getValue() == "map")
		{
			outbuffer.reserve(width * height * 2);
			for (const auto& tile : map)
			{
				outbuffer.push_back((tile.GetTileValue() >> 8) & 0xFF);
				outbuffer.push_back(tile.GetTileValue() & 0xFF);
			}
		}
		else if (outputFormat.getValue() == "lz77")
		{
			std::vector<uint8_t> cmpbuffer;
			cmpbuffer.reserve(width* height * 2);
			outbuffer.assign(width* height * 2, 0);
			for (const auto& tile : map)
			{
				cmpbuffer.push_back((tile.GetTileValue() >> 8) & 0xFF);
				cmpbuffer.push_back(tile.GetTileValue() & 0xFF);
			}
			size_t esize = LZ77::Encode(cmpbuffer.data(), cmpbuffer.size(), outbuffer.data());
			outbuffer.resize(esize);
		}
		else if (outputFormat.getValue() == "rle")
		{
			Tilemap2D map2d(width, height);
			for (size_t y = 0; y < height; ++y)
			{
				for (size_t x = 0; x < width; ++x)
				{
					map2d.SetTile(map[y * width + x], x, y);
				}
			}
			outbuffer = map2d.Compress();
		}
		else if (outputFormat.getValue() == "cbs")
		{
			// Compressed blockset
			if (map.size() % 4 != 0)
			{
				throw std::runtime_error("Cannot convert a tilemap with a size that isn't a multiple of 4 into a blockmap.");
			}
			std::vector<Block> blocks;
			for (auto it = map.cbegin(); it != map.cend(); it += 4)
			{
				blocks.push_back(Block(it, it + 4));
			}
			outbuffer.resize(65536);
			size_t outsize = BlocksetCmp::Encode(blocks, outbuffer.data(), outbuffer.size());
			outbuffer.resize(outsize);
		}
		else if (outputFormat.getValue() == "csv")
		{
			std::stringstream oss;
			oss << std::noskipws;
			size_t elem_count = 0;
			for (const auto& tile : map)
			{
				oss << tile.GetTileValue();
				elem_count++;
				if (elem_count % width == 0)
				{
					oss << std::endl;
				}
				else
				{
					oss << ",";
				}
			}
			std::copy(std::istream_iterator<uint8_t>(oss),
				      std::istream_iterator<uint8_t>(),
				      std::back_inserter(outbuffer));
		}
		else
		{
			throw std::runtime_error("Unexpected output file format");
		}

		// Finally, write-out
		if (output.size() < outOffset.getValue() + outbuffer.size())
		{
			output.resize(outOffset.getValue() + outbuffer.size());
		}
		std::copy(outbuffer.begin(), outbuffer.begin() + outbuffer.size(), output.begin() + outOffset.getValue());

		std::ofstream ofs(fileOut.getValue(), std::ios::binary | std::ios::trunc);
		if (ofs.good() == false)
		{
			std::ostringstream msg;
			msg << "Unable to open output file \"" << fileOut.getValue() << "\" for writing.";
			throw std::runtime_error(msg.str());
		}
		else
		{
			ofs.write(reinterpret_cast<const char*>(output.data()), output.size());
		}
	}
	catch (TCLAP::ArgException& e)
	{
		std::cerr << "Error: '" << e.argId() << "' - " << e.error() << std::endl;
		return 1;
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 2;
	}
	return 0;
}
