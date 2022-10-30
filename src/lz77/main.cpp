#include <iostream>
#include <string>
#include <cstdint>
#include <exception>
#include <fstream>
#include <sstream>
#include <vector>
#include <iterator>

#include <sys/stat.h>

#include <landstalker_tools.h>
#define TCLAP_SETBASE_ZERO 1
#include <tclap/CmdLine.h>
#include <landstalker/LZ77.h>


int main(int argc, char** argv)
{
	try
	{
		TCLAP::CmdLine cmd("Utility for compressing/decompressing LZ77 graphics.\n"
		                   "Part of the landstalker_tools set: github.com/lordmir/landstalker_tools\n"
						   " - Written by LordMir, June 2020",
						   ' ', XSTR(VERSION_MAJOR) "." XSTR(VERSION_MINOR) "." XSTR(VERSION_PATCH));
						   
		TCLAP::UnlabeledValueArg<std::string> fileIn("input_file", "The input file (.lz77/.bin)", true, "", "in_filename");
		TCLAP::UnlabeledValueArg<std::string> fileOut("output_file", "The output file (.lz77/.bin)", true, "", "out_filename");
		TCLAP::SwitchArg decompress("d", "decompress", "Decompress [input_file], and write result to [output_file]", false);
		TCLAP::SwitchArg compress("c", "compress", "Compress [input_file] and write result to [output_file]", false);
		TCLAP::SwitchArg force("f", "force", "Force overwrite if file already exists and no offset has been set", false);
		TCLAP::ValueArg<uint32_t> inOffset("i", "inoffset", "Offset into the input file to start reading data, useful if working with the raw ROM", false, 0, "offset");
		TCLAP::ValueArg<uint32_t> outOffset("o", "outoffset", "Offset into the output file to start writing data, useful if working with the raw ROM.\n"
			                                       "**WARNING** This program will not make any attempt to rearrange data in the ROM. If the compressed "
			                                       "size is greater than expected, then data could be overwritten!", false, 0, "offset");
		cmd.xorAdd(decompress, compress);
		cmd.add(force);
		cmd.add(fileIn);
		cmd.add(fileOut);
		cmd.add(inOffset);
		cmd.add(outOffset);
		cmd.parse(argc, argv);

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

		// Next, the compression/decompression
		outbuffer.assign(65536, 0);
		size_t outlen = 0;
		size_t inlen = input.size() - inOffset.getValue();

		if (decompress.isSet() == true)
		{
			outlen = LZ77::Decode(input.data() + inOffset.getValue(), input.size() - inOffset.getValue(), outbuffer.data(), inlen);
		}
		else
		{
			outlen = LZ77::Encode(input.data() + inOffset.getValue(), input.size() - inOffset.getValue(), outbuffer.data());
		}

		// Finally, write-out
		if (output.size() < outOffset.getValue() + outlen)
		{
			output.resize(outOffset.getValue() + outlen);
		}
		std::copy(outbuffer.begin(), outbuffer.begin() + outlen, output.begin() + outOffset.getValue());
		
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
		std::cout << "Wrote " << outlen << " bytes of " << (decompress.getValue() ? "decompressed" : "compressed") << " data to file \"" << fileOut.getValue() << "\"." << std::endl;
		std::cout << "Original data was " << inlen << " bytes, with a total compression ratio of " << 100.0 * (decompress.getValue() ? static_cast<double>(inlen)/outlen : static_cast<double>(outlen) / inlen) << "%" << std::endl;
	}
	catch(TCLAP::ArgException& e)
	{
		std::cerr << "Error: '" << e.argId() << "' - " << e.error() << std::endl;
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}
