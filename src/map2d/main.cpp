#include <iostream>
#include <string>
#include <cstdint>
#include <exception>
#include <fstream>
#include <sstream>
#include <vector>
#include <iterator>
#include <memory>
#include <sys/stat.h>

#include <landstalker_tools.h>
#define TCLAP_SETBASE_ZERO 1
#include <tclap/CmdLine.h>
#include <rapidcsv.h>
#include <landstalker/misc/LZ77.h>
#include <landstalker/2d_maps/Tilemap2DRLE.h>
#include <landstalker/tileset/Tile.h>
#include <landstalker/blockset/Block.h>
#include <landstalker/blockset/BlocksetCmp.h>

bool validateParams(const std::string& format_in, TCLAP::ValueArg<uint32_t>& offset_in, TCLAP::ValueArg<uint32_t>& width_in, TCLAP::ValueArg<uint32_t>& height_in, uint32_t& width_out, uint32_t& height_out)
{
	if (format_in == "map")
	{
		if (width_in.getValue() <= 0 && offset_in.isSet() == false)
		{
			throw std::runtime_error("Error: a valid width must be specified when converting binary or LZ77 compressed maps.");
		}
		if ((width_in.getValue() <= 0 || height_in.getValue()) && offset_in.isSet() == true)
		{
			throw std::runtime_error("Error: a valid width and height must be specified when reading binary or LZ77 compressed maps from ROM.");
		}
	}
	else
	{
		if (width_in.getValue() != 0 || height_in.getValue() != 0)
		{
			std::cerr << "Warning: Width and height parameters will be ignored, as " << format_in
			          << " format maps already contain this information." << std::endl;
		}
		if (format_in == "cbs")
		{
			// Blocksets are essentially a list of four tiles. Each four tiles makes up a single 2x2 block
			width_out = 4;
			height_out = 0;
		}
	}
	return true;
}

bool readFile(std::string filename, std::vector<uint8_t>& data, const std::string& format = "map", uint32_t offset = 0, std::size_t width = 0, std::size_t height = 0)
{
	std::size_t expected_file_size = 0;
	if (format == "map")
	{
		expected_file_size = width * height * 2;
	}
	std::ifstream ifs(filename, std::ios::binary);
	if (ifs.good() == false)
	{
		std::ostringstream msg;
		msg << "Unable to open file \"" << filename << "\" for reading.";
		throw std::runtime_error(msg.str());
	}
	else
	{
		ifs.unsetf(std::ios::skipws);

		ifs.seekg(0, std::ios::end);
		std::streampos filesize = ifs.tellg();
		ifs.seekg(0, std::ios::beg);

		if (filesize > 0 && offset >= filesize)
		{
			std::ostringstream msg;
			msg << "Provided offset " << offset << " is greater than the size of the file \""
				<< filename << "\" (" << filesize << " bytes).";
			throw std::runtime_error(msg.str());
		}
		if (static_cast<int>(expected_file_size) > (static_cast<int>(filesize) - offset))
		{
			std::ostringstream msg;
			msg << "Expected map size (" << expected_file_size << " bytes) is greater than the available size of the file \""
				<< filename << "\" (" << (static_cast<int>(filesize) - offset) << " bytes).";
			throw std::runtime_error(msg.str());
		}

		data.clear();
		data.reserve(static_cast<unsigned int>(filesize));
		data.insert(data.begin(), std::istream_iterator<uint8_t>(ifs), std::istream_iterator<uint8_t>());
	}
	ifs.close();
	return true;
}

bool processInputFile(std::vector<uint8_t>& data, std::unique_ptr<Landstalker::Tilemap2D>& map2d, const std::string& format = "map", std::size_t base = 0, std::size_t width = 0, std::size_t height = 0)
{
	if (format == "map")
	{
		// If height not specified, calculate it based on the file size
		if (width > 0 && height == 0)
		{
			height = (data.size() + width * 2 - 1) / (width * 2);
		}
		if (data.size() % (width * 2) > 0)
		{
			std::cerr << "Warning: file size (" << data.size() << " bytes) is not an exact multiple of the map width. Excess bytes will be ignored." << std::endl;
			data.resize(width * height * 2);
		}
		map2d = std::make_unique<Landstalker::Tilemap2D>(data, width, height, Landstalker::Tilemap2D::Compression::NONE, base);
	}
	else if (format == "lz77")
	{
		map2d = std::make_unique<Landstalker::Tilemap2D>(data, Landstalker::Tilemap2D::Compression::LZ77, base);
	}
	else if (format == "rle")
	{
		map2d = std::make_unique<Landstalker::Tilemap2D>(data, Landstalker::Tilemap2D::Compression::RLE, base);
	}
	else if (format == "cbs")
	{
		// Compressed blockset
		std::vector<Landstalker::MapBlock> blocks;
		height = Landstalker::BlocksetCmp::Decode(data.data(), data.size(), blocks);
		if (height > 0)
		{
			width = 4;
			map2d = std::make_unique<Landstalker::Tilemap2D>(width, height);
			for (size_t y = 0; y < blocks.size(); ++y)
			{
				for (size_t x = 0; x < width; ++x)
				{
					map2d->SetTile(blocks[y].GetTile(x), x, y);
				}
			}
		}
		else
		{
			map2d = std::make_unique<Landstalker::Tilemap2D>(0, 0);
		}
	}
	else if (format == "csv")
	{
		if (data.size() == 0)
		{
			// Special case - zero byte file.
			map2d = std::make_unique<Landstalker::Tilemap2D>(0, 0);
		}
		else
		{
			std::stringstream ss(std::string(data.begin(), data.end()));
			rapidcsv::Document csv(ss, rapidcsv::LabelParams(-1, -1));
			// For CSV, first work out the number of rows and columns
			width = csv.GetColumnCount();
			height = csv.GetRowCount();
			if (width <= 0 || height <= 0)
			{
				throw std::runtime_error("Error: CSV malformed");
			}
			map2d = std::make_unique<Landstalker::Tilemap2D>(width, height, base);
			for (size_t y = 0; y < height; y++)
			{
				for (size_t x = 0; x < width; x++)
				{
					map2d->SetTile(csv.GetCell<unsigned>(x, y), x, y);
				}
			}
		}
	}
	else
	{
		throw std::runtime_error("Unexpected input file format");
	}
	return true;
}

bool validateOutputFile(const std::string& filename, TCLAP::ValueArg<uint32_t>& offset, bool force)
{
	std::ifstream outfile(filename, std::ios::binary);
	if (outfile.good() == false)
	{
		if (offset.isSet() == true)
		{
			std::ostringstream msg;
			msg << "Unable to write to offset as file \"" << filename << "\" can't be opened.";
			throw std::runtime_error(msg.str());
		}
	}
	else
	{
		// Writing to an offset? Cache our output file
		if (offset.isSet() == true)
		{
		}
		else if (force == false)
		{
			std::ostringstream msg;
			msg << "Unable to write to file \"" << filename << "\" as it already exists. Try running the command again with the -f flag.";
			throw std::runtime_error(msg.str());
		}
	}
	outfile.close();
	return true;
}

bool convertMap(std::vector<uint8_t>& outbuffer, std::unique_ptr<Landstalker::Tilemap2D>& map2d, const std::string& format, std::size_t left = 0, std::size_t top = 0)
{
	const std::size_t width = map2d->GetWidth();
	const std::size_t height = map2d->GetHeight();
	const std::size_t size = width * height * 2;
	map2d->SetLeft(left & 0xFF);
	map2d->SetTop(top & 0xFF);

	outbuffer.reserve(width * height * 2);

	if (format == "map")
	{
		map2d->GetBits(outbuffer, Landstalker::Tilemap2D::Compression::NONE);
	}
	else if (format == "rle")
	{
		map2d->GetBits(outbuffer, Landstalker::Tilemap2D::Compression::RLE);
	}
	else if (format == "lz77")
	{
		map2d->GetBits(outbuffer, Landstalker::Tilemap2D::Compression::LZ77);
	}
	else if (format == "cbs")
	{
		// Compressed blockset
		std::vector<Landstalker::MapBlock> blocks;
		for (std::size_t y = 0; y < height; ++y)
		{
			std::vector<Landstalker::Tile> block(4);
			for (std::size_t x = 0; x < width; ++x)
			{
				block[x] = map2d->GetTile(x, y);
			}
			blocks.push_back(Landstalker::MapBlock(block.begin(), block.end()));
		}
		outbuffer.resize(65536);
		size_t outsize = Landstalker::BlocksetCmp::Encode(blocks, outbuffer.data(), outbuffer.size());
		outbuffer.resize(outsize);
	}
	else if (format == "csv")
	{
		std::stringstream oss;
		oss << std::noskipws;
		size_t elem_count = 0;
		for (std::size_t y = 0; y < height; ++y)
		{
			for (std::size_t x = 0; x < width; ++x)
			{
				oss << map2d->GetTile(x, y).GetTileValue();
				if (x == width - 1)
				{
					oss << std::endl;
				}
				else
				{
					oss << ",";
				}
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
	return true;
}

bool write(const std::string& filename, const std::vector<uint8_t>& output)
{
	std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
	if (ofs.good() == false)
	{
		std::ostringstream msg;
		msg << "Unable to open output file \"" << filename << "\" for writing.";
		throw std::runtime_error(msg.str());
	}
	else
	{
		ofs.write(reinterpret_cast<const char*>(output.data()), output.size());
	}
	return true;
}

bool writeOut(const std::string& filename, const std::vector<uint8_t>& output, bool insert, uint32_t offset)
{
	if (insert == true)
	{
		std::vector<uint8_t> outfile;
		std::ifstream ifs(filename, std::ios::binary);
		if (ifs.good() == false)
		{
			std::ostringstream msg;
			msg << "Unable to open output file \"" << filename << "\" for reading.";
			throw std::runtime_error(msg.str());
		}
		ifs.unsetf(std::ios::skipws);

		ifs.seekg(0, std::ios::end);
		std::streampos filesize = ifs.tellg();
		ifs.seekg(0, std::ios::beg);

		outfile.reserve(static_cast<unsigned int>(filesize));
		outfile.insert(outfile.begin(), std::istream_iterator<uint8_t>(ifs), std::istream_iterator<uint8_t>());
		ifs.close();
		if (outfile.size() < offset + output.size())
		{
			outfile.resize(offset + outfile.size());
		}
		std::copy(output.begin(), output.end(), outfile.begin() + offset);
		return write(filename, outfile);
	}
	else
	{
		return write(filename, output);
	}
}

int main(int argc, char** argv)
{
	try
	{
		TCLAP::CmdLine cmd("Utility for converting 2D tilemaps and blocksets between binary, compressed binary and CSV.\n"
			"Part of the landstalker_tools set: github.com/lordmir/landstalker_tools\n"
			" - Written by LordMir, June 2020",
			' ', XSTR(VERSION_MAJOR) "." XSTR(VERSION_MINOR) "." XSTR(VERSION_PATCH));

		std::vector<std::string> formats{"csv","map","lz77","rle","cbs"};
		TCLAP::ValuesConstraint<std::string> allowedVals(formats);

		TCLAP::UnlabeledValueArg<std::string> fileIn("input_file", "The input file (.map/.rle/.lz77/.csv/.cbs)", true, "", "in_filename");
		TCLAP::UnlabeledValueArg<std::string> fileOut("output_file", "The output file (.map/.rle/.lz77/.csv/.cbs)", true, "", "out_filename");
		TCLAP::ValueArg<std::string> inputFormat("i", "input_format", "Input format", true, "map", &allowedVals);
		TCLAP::ValueArg<std::string> outputFormat("o", "output_format", "Output format", true, "csv", &allowedVals);
		TCLAP::ValueArg<uint32_t> widthIn("w", "width", "Width of the 2D map in 8x8 tiles", false, 0, "width_tiles");
		TCLAP::ValueArg<uint32_t> heightIn("", "height", "Height of the 2D map in 8x8 tiles", false, 0, "height_tiles");
		TCLAP::ValueArg<uint32_t> leftIn("l", "left", "Left coordinate of the 2D map in 8x8 tiles", false, 0, "left_tiles");
		TCLAP::ValueArg<uint32_t> topIn("t", "top", "Top coordinate of the 2D map in 8x8 tiles", false, 0, "top_tiles");
		TCLAP::ValueArg<uint32_t> tileBaseIn("b", "base", "The tile base for the 2D map", false, 0, "tile_base");
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
		cmd.add(leftIn);
		cmd.add(topIn);
		cmd.add(tileBaseIn);
		cmd.add(inOffset);
		cmd.add(outOffset);
		cmd.parse(argc, argv);

		std::vector<uint8_t> input;
		uint32_t width = widthIn.getValue();
		uint32_t height = heightIn.getValue();
		uint32_t expected_input_size = 0;
		std::unique_ptr<Landstalker::Tilemap2D> map2d;

		validateParams(inputFormat.getValue(), inOffset, widthIn, heightIn, width, height);


		// First, cache our input file
		readFile(fileIn.getValue(), input, inputFormat.getValue(), inOffset.getValue(), width, height);

		// Next, test our output file
		validateOutputFile(fileOut.getValue(), outOffset, force.getValue());

		std::vector<uint8_t> map_data(input.begin() + inOffset.getValue(), input.end());
		std::vector<uint8_t> output;
		// Next, the conversion. Convert input to intermeditate binary
		processInputFile(map_data, map2d, inputFormat.getValue(), tileBaseIn.getValue(), width, height);
		
		// Convert intermediate binary to output
		std::cout << "Writing " << map2d->GetWidth() << "x" << map2d->GetHeight() << " tilemap (" 
		          << map2d->GetLeft() << ", " << map2d->GetTop() << ")" << std::endl;
		convertMap(output, map2d, outputFormat.getValue(), leftIn.getValue(), topIn.getValue());

		// Finally, write-out
		writeOut(fileOut.getValue(), output, outOffset.isSet(), outOffset.getValue());
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
