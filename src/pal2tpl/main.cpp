#include <iostream>
#include <string>
#include <cstdint>
#include <exception>
#include <fstream>
#include <sstream>
#include <vector>
#include <iterator>
#include <cstring>

#include <sys/stat.h>

#include <landstalker_tools.h>
#define TCLAP_SETBASE_ZERO 1
#include <tclap/CmdLine.h>

const char* TPLPAL_MAGIC = "TPL";

uint32_t GREY = 0xD8D8D8;
uint32_t BLACK = 0x000000;

void Gen2Tpl(uint8_t* tpl, const uint8_t* gen)
{
	tpl[0] = (gen[1] & 0x0E) * 18;
	tpl[1] = ((gen[1] & 0xE0) >> 4) * 18;
	tpl[2] = (gen[0] & 0x0E) * 18;
}

void Tpl2Gen(uint8_t* gen, const uint8_t* tpl)
{
	gen[0] =  (tpl[2] /18) & 0x0E;
	gen[1] = ((tpl[1] /18) & 0x0E) << 4;
	gen[1] |= (tpl[0] /18) & 0x0E;
}

void rgb2Gen(uint8_t* gen, uint32_t rgb)
{
	gen[0] = ((rgb & 0x0000FF) / 18) & 0x0E;
	gen[1] = ((((rgb & 0x00FF00) >> 8) / 18) & 0x0E) << 4;
	gen[1] |= (((rgb & 0xFF0000) >> 16) / 18) & 0x0E;
}

void rgb2Tpl(uint8_t* tpl, uint32_t rgb)
{
	tpl[0] = rgb & 0x0000FF;
	tpl[1] = (rgb & 0x00FF00) >> 8;
	tpl[2] = (rgb & 0xFF0000) >> 16;
}

int main(int argc, char** argv)
{
	try
	{
		TCLAP::CmdLine cmd("Utility for converting Genesis / Landstalker palettes to/from Tile Layer Pro format.\n"
			"Part of the landstalker_tools set: github.com/lordmir/landstalker_tools\n"
			" - Written by LordMir, June 2020",
			' ', XSTR(VERSION_MAJOR) "." XSTR(VERSION_MINOR) "." XSTR(VERSION_PATCH));

		TCLAP::UnlabeledValueArg<std::string> fileIn("input_file", "The input file (.pal/.tpl)", true, "", "in_filename");
		TCLAP::UnlabeledValueArg<std::string> fileOut("output_file", "The output file (.pal/.tpl)", true, "", "out_filename");
		TCLAP::SwitchArg toTpl("t", "toTPL", "Convert input file from Genesis format to TPL format", false);
		TCLAP::SwitchArg toGen("g", "toGen", "Convert input file from TPL format to Genesis format", false);
		TCLAP::SwitchArg force("f", "force", "Force overwrite if file already exists and no offset has been set", false);
		TCLAP::SwitchArg encodeLength("z", "encode_length", "Read/write palette length at start of file (GEN)", false);
		TCLAP::ValueArg<uint32_t> start("s", "start", "The starting index into the palette", false, 0, "start_index");
		TCLAP::ValueArg<uint32_t> length("l", "length", "The number of entries in the palette. Setting this parameter to zero will load as many entries as possible.", false, 0, "num_entries");
		TCLAP::ValueArg<uint32_t> count("c", "count", "The total number of palettes to load", false, 1, "num_palettes");
		TCLAP::SwitchArg init("n", "init", "initialises the standard Landstalker palette entries (0 = transparent, 1 = light grey, 15 = black)", true);
		TCLAP::ValueArg<uint32_t> transparentColour("r", "transparent", "The colour to use for transparency, in hex RGB format. By default, this is black (0x000000)", false, BLACK, "t_colour");
		TCLAP::ValueArg<uint32_t> inOffset("i", "inoffset", "Offset into the input file to start reading data, useful if working with the raw ROM", false, 0, "offset");
		TCLAP::ValueArg<uint32_t> outOffset("o", "outoffset", "Offset into the output file to start writing data, useful if working with the raw ROM.\n"
			"**WARNING** This program will not make any attempt to rearrange data in the ROM. If the new "
			"size is greater than expected, then data could be overwritten!", false, 0, "offset");
		cmd.xorAdd(toTpl, toGen);
		cmd.add(force);
		cmd.add(encodeLength);
		cmd.add(fileIn);
		cmd.add(fileOut);
		cmd.add(inOffset);
		cmd.add(outOffset);
		cmd.add(start);
		cmd.add(length);
		cmd.add(count);
		cmd.add(init);
		cmd.add(transparentColour);
		cmd.parse(argc, argv);

		// If we request n palettes containing m entries, then we expect to find m*n colours
		size_t len = length.getValue();
		size_t expected_entries = count.getValue() * len;
		// If we are reading a Genesis type palette, then the size to read is simply double the number of colours (each colour is stored as 2 bytes)
		size_t gen_pal_size = expected_entries * 2;
		// If we are reading a TPL palette, then we work in blocks of 16 colours - expect a full 16 colour palette for each requested palette.
		// A TPL palette uses 3 bytes per colour, and also contains a 4-character magic word at the start.
		size_t tpl_pal_size = ((expected_entries + 15)/16) * 16 * 3 * count.getValue() + strlen(TPLPAL_MAGIC) + 1;
		if (tpl_pal_size == strlen(TPLPAL_MAGIC) + 1)
		{
			tpl_pal_size += 16 * 3;
		}
		// Define our expected input and output sizes depending on the conversion direction
		size_t expected_size = toTpl.getValue() ? gen_pal_size : tpl_pal_size;
		size_t out_size = toTpl.getValue() ? tpl_pal_size : gen_pal_size;

		// Setting length to zero tells the program to read in as many colours as possible. This will not work with the -c option, as the program will read
		// in as many palettes as exist.
		if (length.getValue() == 0)
		{
			if (count.getValue() > 1)
			{
				throw std::runtime_error("It is necessary to specify the palette size using the -l option if you wish to convert multiple palettes.");
			}
		}

		std::vector<uint8_t> input;
		std::vector<uint8_t> output;
		std::vector<uint8_t> outbuffer;

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

			if (inOffset.getValue() >= filesize)
			{
				std::ostringstream msg;
				msg << "Provided offset " << inOffset.getValue() << " is greater than the size of the file \"" << fileIn.getValue() << "\" (" << filesize << " bytes).";
				throw std::runtime_error(msg.str());
			}

			if (toTpl.getValue() == true && encodeLength.isSet() == true)
			{
				if (filesize > 2)
				{
					uint16_t tmpw;
					char tmpb;
					ifs.read(&tmpb, 1);
					tmpw = tmpb << 8;
					ifs.read(&tmpb, 1);
					tmpw |= tmpb;
					expected_entries = (tmpw + 1);
					expected_size = expected_entries * 2 + 2;
					ifs.seekg(0, std::ios::beg);
				}
				else
				{
					throw std::runtime_error("Input file size is not big enough to contain all requested entries.");
				}
			}

			if (filesize > expected_size && expected_size != 0)
			{
				std::cerr << "WARNING: Input file size (" << filesize << " bytes) is bigger than expected (" << expected_size << " bytes). Trailing bytes will be ignored." << std::endl;
			}
			else if (filesize < expected_size)
			{
				throw std::runtime_error("Input file size is not big enough to contain all requested entries.");
			}

			input.reserve(static_cast<unsigned int>(filesize));
			input.insert(input.begin(), std::istream_iterator<uint8_t>(ifs), std::istream_iterator<uint8_t>());
		}
		ifs.close();
		// The expected entries being set to zero tells the program to read in as many colours as possible.
		if (expected_entries == 0)
		{
			expected_entries = toTpl.getValue() ? (input.size() / 2) : ((input.size() - 4) / 3);
		}

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

		// Next, the conversion
		gen_pal_size = expected_entries * 2 + (encodeLength.isSet() ? 2 : 0);
		tpl_pal_size = ((expected_entries + 15) / 16) * 16 * count.getValue() * 3 + strlen(TPLPAL_MAGIC) + 1;
		expected_size = toTpl.getValue() ? gen_pal_size : tpl_pal_size;
		out_size = toTpl.getValue() ? tpl_pal_size : gen_pal_size;
		size_t outlen = 0;
		size_t inlen = input.size() - inOffset.getValue();
		size_t entries_per_pal = length.getValue() > 0 ? length.getValue() : expected_entries;
		outbuffer.assign(out_size, 0);
		
		if (toTpl.isSet() == true)
		{
			uint8_t* out = outbuffer.data();
			uint8_t* in = input.data();
			if (encodeLength.isSet() == true)
			{
				in += 2;
			}
			memcpy(out, TPLPAL_MAGIC, 4);
			out += 4;
			for(size_t p = 0; p < count.getValue(); ++p)
			{
				if (init.getValue())
				{
					rgb2Tpl(out, transparentColour.getValue());
					rgb2Tpl(out + 1 * 3, GREY);
					rgb2Tpl(out + 15 * 3, BLACK);
				}

				for (size_t i = 0; i < entries_per_pal; ++i)
				{
					Gen2Tpl(out + (start.getValue() + i) * 3, in);
					in += 2;
				}

				out += 16 * 3;
			}
		}
		else
		{
			uint8_t* out = outbuffer.data();
			uint8_t* in = input.data() + 4;
			if (encodeLength.isSet() == true)
			{
				out[0] = ((entries_per_pal - 1) >> 8) & 0xFF;
				out[1] = (entries_per_pal - 1) & 0xFF;
				out += 2;
			}
			for (size_t p = 0; p < count.getValue(); ++p)
			{
				for (size_t i = 0; i < entries_per_pal; ++i)
				{
					Tpl2Gen(out, in + (start.getValue() + i) * 3);
					out += 2;
				}
				in += 16 * 3;
			}
		}

		// Finally, write-out
		if (output.size() < outOffset.getValue() + outbuffer.size())
		{
			output.resize(outOffset.getValue() + outbuffer.size());
		}
		std::copy(outbuffer.begin(), outbuffer.end(), output.begin() + outOffset.getValue());

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
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}
