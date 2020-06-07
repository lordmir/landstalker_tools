#include <iostream>
#include <string>
#include <cstdint>
#include <exception>
#include <fstream>
#include <sstream>
#include <vector>
#include <iterator>
#include <set>
#include <cstdint>

#include <sys/stat.h>

#define TCLAP_SETBASE_ZERO 1
#include <tclap/CmdLine.h>
#include <rapidcsv.h>
#include <Tilemap3DCmp.h>

bool fileExists(const std::string& filename)
{
	struct stat buffer;
	return (stat(filename.c_str(), &buffer) == 0);
}

std::fstream openCSVFile(const std::string& filename, bool write, bool force)
{
	bool exists = fileExists(filename);
	std::fstream fs;

	if (exists == false && write == false)
	{
		std::ostringstream msg;
		msg << "Unable to open file \"" << filename << "\" for reading.";
		throw std::runtime_error(msg.str());
	}
	else if (exists == true && write == true && force == false)
	{
		std::ostringstream msg;
		msg << "Unable to write to file \"" << filename << "\" as it already exists. Try running the command again with the -f flag.";
		throw std::runtime_error(msg.str());
	}

	if (write == true)
	{
		fs.open(filename, std::ios::out);
		if (fs.good() == false)
		{
			std::ostringstream msg;
			msg << "Unable to open file \"" << filename << "\" for writing.";
			throw std::runtime_error(msg.str());
		}
	}
	else
	{
		fs.open(filename, std::ios::in);
		if (fs.good() == false)
		{
			std::ostringstream msg;
			msg << "Unable to open file \"" << filename << "\" for reading.";
			throw std::runtime_error(msg.str());
		}
	}

	return fs;
}

void ConvertMapToCSV(const RoomTilemap& rt, std::ostream& bg, std::ostream& fg, std::ostream& hm)
{
	for (int i = 0; i < rt.width * rt.height; ++i)
	{
		fg << rt.foreground[i];
		bg << rt.background[i];
		if ((i + 1) % rt.width == 0)
		{
			fg << std::endl;
			bg << std::endl;
		}
		else
		{
			fg << ",";
			bg << ",";
		}
	}
	hm << static_cast<int>(rt.left) << "," << static_cast<int>(rt.top) << std::endl;
	for (int i = 0; i < rt.hmwidth * rt.hmheight; ++i)
	{
		hm << rt.heightmap[i];
		if ((i + 1) % rt.hmwidth == 0)
		{
			hm << std::endl;
		}
		else
		{
			hm << ",";
		}
	}
}

RoomTilemap GetMapFromCSV(std::istream& bg, std::istream& fg, std::istream& hm)
{
	RoomTilemap rt;

	rapidcsv::Document fgCsv(fg, rapidcsv::LabelParams(-1, -1), rapidcsv::SeparatorParams(), rapidcsv::ConverterParams(true, -1.0, -1));
	rapidcsv::Document bgCsv(bg, rapidcsv::LabelParams(-1, -1), rapidcsv::SeparatorParams(), rapidcsv::ConverterParams(true, -1.0, -1));
	rapidcsv::Document hmCsv(hm, rapidcsv::LabelParams(-1, -1), rapidcsv::SeparatorParams(), rapidcsv::ConverterParams(true, -1.0, -1));
	rt.width = static_cast<uint8_t>(fgCsv.GetColumnCount());
	rt.height = static_cast<uint8_t>(fgCsv.GetRowCount());
	// Test to see if last row is empty
	while (fgCsv.GetRow<std::string>(rt.height - 1).size() < rt.width)
	{
		rt.height--;
	}
	uint8_t bgwidth = static_cast<uint8_t>(bgCsv.GetColumnCount());
	uint8_t bgheight = static_cast<uint8_t>(bgCsv.GetRowCount());
	while (bgCsv.GetRow<std::string>(bgheight - 1).size() < bgwidth)
	{
		bgheight--;
	}
	if ((rt.width <= 0 || rt.height <= 0) || (rt.width != bgwidth || rt.height != bgheight))
	{
		throw std::runtime_error("Error: CSV malformed");
	}
	rt.hmheight = static_cast<uint8_t>(hmCsv.GetRowCount()) - 1;
	rt.hmwidth = static_cast<uint8_t>(hmCsv.GetRow<std::string>(1).size());
	while (hmCsv.GetRow<std::string>(rt.hmheight).size() < rt.hmwidth)
	{
		rt.hmheight--;
	}
	if (rt.hmwidth <= 0 || rt.hmheight <= 0)
	{
		throw std::runtime_error("Error: CSV malformed");
	}
	int left = hmCsv.GetCell<int>(0, 0);
	int top = hmCsv.GetCell<int>(1, 0);
	if (left < 0 || top < 0)
	{
		throw std::runtime_error("Error: CSV malformed");
	}
	rt.left = static_cast<uint8_t>(left);
	rt.top = static_cast<uint8_t>(top);
	std::cout << "Map: " << (int)rt.width << "x" << (int)rt.height << " " << (int)rt.left << "," << (int)rt.top
		<< " Heightmap: " << (int)rt.hmwidth << "x" << (int)rt.hmheight << std::endl;
	for (size_t y = 0; y < rt.height; ++y)
	{
		for (size_t x = 0; x < rt.width; ++x)
		{
			int btemp = bgCsv.GetCell<int>(x, y);
			int ftemp = fgCsv.GetCell<int>(x, y);
			if (btemp == -1 || ftemp == -1)
			{
				throw std::runtime_error("Error: CSV malformed");
			}
			rt.foreground.push_back(static_cast<uint16_t>(ftemp));
			rt.background.push_back(static_cast<uint16_t>(btemp));
		}
	}
	for (size_t y = 1; y <= rt.hmheight; ++y)
	{
		for (size_t x = 0; x < rt.hmwidth; ++x)
		{
			int htemp = hmCsv.GetCell<int>(x, y);
			if (htemp == -1)
			{
				throw std::runtime_error("Error: CSV malformed");
			}
			rt.heightmap.push_back(static_cast<uint16_t>(htemp));
		}
	}

	return rt;
}

int romtest(const std::string& infilename, const std::string& outfilename)
{
	std::vector<uint8_t> rom;
	std::ifstream ifs;
	ifs.open(infilename, std::ios::binary);
	ifs.unsetf(std::ios::skipws);
	rom.insert(rom.begin(),
		std::istream_iterator<uint8_t>(ifs),
		std::istream_iterator<uint8_t>());
	ifs.close();
	int passes = 0;
	int fails = 0;
	int size = 0;
	int uncompressed_size = 0;
	int orig_size = 500015;
	std::set<uint32_t> map_offsets;
	std::vector<std::vector<uint8_t>> cmaps;
	uint8_t* o = rom.data() + 0xA0A12;
	for (size_t i = 0; i < 816; ++i)
	{
		uint8_t* op = o + i * 8;
		uint32_t os = op[0] << 24 | op[1] << 16 | op[2] << 8 | op[3];
		map_offsets.insert(os);
	}
	for (auto it = map_offsets.begin(); it != map_offsets.end(); ++it)
	{
		std::cout << std::hex << *it << std::dec << std::endl;
		RoomTilemap rt;
		Tilemap3DCmp::Decode(rom.data() + *it, rt);

		std::cout << (unsigned)rt.GetWidth() << "x" << (unsigned)rt.GetHeight() << ": " << rt.background.size() << std::endl;

		std::vector<uint8_t> result(65536);
		std::ofstream bgofs("bg.csv");
		std::ofstream fgofs("fg.csv");
		std::ofstream hmofs("hm.csv");
		ConvertMapToCSV(rt, bgofs, fgofs, hmofs);
		std::ifstream bgifs("bg.csv");
		std::ifstream fgifs("fg.csv");
		std::ifstream hmifs("hm.csv");
		RoomTilemap rtt = GetMapFromCSV(bgifs, fgifs, hmifs);
		result.resize(Tilemap3DCmp::Encode(rtt,result.data(),result.size()));
		std::cout << "Recompressed size " << result.size() << " bytes." << std::endl;
		RoomTilemap rt2;
		Tilemap3DCmp::Decode(result.data(), rt2);
		uncompressed_size += rt.background.size() * 4 + rt.heightmap.size() * 2 + 6;
		size += result.size();
		std::cout << (unsigned)rt.GetWidth() << "x" << (unsigned)rt.GetHeight() << ": " << rt.background.size() << std::endl;

		bool pass = ((rt.GetWidth() == rt2.GetWidth()) &&
			(rt.GetHeight() == rt2.GetHeight()) &&
			(rt.GetLeft() == rt2.GetLeft()) &&
			(rt.GetTop() == rt2.GetTop()) &&
			(rt.hmwidth == rt2.hmwidth) &&
			(rt.hmheight == rt2.hmheight) &&
			(rt.background == rt2.background) &&
			(rt.foreground == rt2.foreground) &&
			(rt.heightmap == rt2.heightmap));
		if (pass == false)
		{
			std::cout << "Width:      " << (rt.GetWidth() == rt2.GetWidth() ? "OK" : "MISMATCH") << std::endl;
			std::cout << "Height:     " << (rt.GetHeight() == rt2.GetHeight() ? "OK" : "MISMATCH") << std::endl;
			std::cout << "Left:       " << (rt.GetLeft() == rt2.GetLeft() ? "OK" : "MISMATCH") << std::endl;
			std::cout << "Top:        " << (rt.GetTop() == rt2.GetTop() ? "OK" : "MISMATCH") << std::endl;
			std::cout << "HM-Width:   " << (rt.hmwidth == rt2.hmwidth ? "OK" : "MISMATCH") << std::endl;
			std::cout << "HM-Height:  " << (rt.hmheight == rt2.hmheight ? "OK" : "MISMATCH") << std::endl;
			std::cout << "Background: " << (rt.background == rt2.background ? "OK" : "MISMATCH") << std::endl;
			std::cout << "Foreground: " << (rt.foreground == rt2.foreground ? "OK" : "MISMATCH") << std::endl;
			std::cout << "Heightmap:  " << (rt.heightmap == rt2.heightmap ? "OK" : "MISMATCH") << std::endl;
		}
		std::cout << "Overall:   " << (pass ? "** OK **" : "** FAIL **") << std::endl;
		cmaps.push_back(result);
		if (pass)
		{
			passes++;
		}
		else
		{
			fails++;
		}
	}

	std::map<uint32_t, int> map_lookup;
	size_t map_counter = 0;
	size_t map_offset = 0;
	for (auto it = map_offsets.cbegin(); it != map_offsets.cend(); ++it)
	{
		map_lookup[*it] = map_offset;
		map_offset += cmaps[map_counter++].size();
	}
	for (const auto ml : map_lookup)
	{
		std::cout << std::hex << ml.first << " --> " << std::dec << ml.second << std::endl;
	}
	std::vector<uint8_t> roomlist;
	uint32_t new_map_offset = 0x220000;
	o = rom.data() + 0xA0A12;
	for (size_t i = 0; i < 816; ++i)
	{
		uint32_t old_os = o[0] << 24 | o[1] << 16 | o[2] << 8 | o[3];
		uint32_t new_os = map_lookup[old_os] + new_map_offset;
		roomlist.push_back((new_os >> 24) & 0xFF);
		roomlist.push_back((new_os >> 16) & 0xFF);
		roomlist.push_back((new_os >> 8) & 0xFF);
		roomlist.push_back(new_os & 0xFF);
		roomlist.push_back(o[4]);
		roomlist.push_back(o[5]);
		roomlist.push_back(o[6]);
		roomlist.push_back(o[7]);
		o += 8;
	}

	std::cout << "*** DONE! ***" << std::endl;
	std::cout << "PASSES:            " << std::dec << std::setw(10) << passes << std::setw(10) << std::endl;
	std::cout << "FAILS:             " << std::dec << std::setw(10) << fails << std::setw(10) << std::endl;
	std::cout << "TOTAL SIZE:        " << std::dec << std::setw(10) << size << std::setw(10) << std::endl;
	std::cout << "UNCOMPRESSED SIZE: " << std::dec << std::setw(10) << uncompressed_size << std::setw(10) << (100.0 * size / uncompressed_size) << std::endl;
	std::cout << "ORIGINAL SIZE:     " << std::dec << std::setw(10) << orig_size << std::setw(10) << (100.0 * size / orig_size) << std::endl;

	rom[0x210] = 0x00;
	rom[0x211] = 0x00;
	rom[0xA0A00] = 0x00;
	rom[0xA0A01] = 0x21;
	rom[0xA0A02] = 0x00;
	rom[0xA0A03] = 0x00;
	rom.resize(0x210000);
	rom.reserve(0x400000);
	rom.insert(rom.end(), roomlist.begin(), roomlist.end());
	rom.resize(0x220000);
	for (const auto& m : cmaps)
	{
		rom.insert(rom.end(), m.begin(), m.end());
	}
	rom.resize(0x400000);
	std::ostringstream ss;
	std::ofstream outfile(outfilename, std::ios::out | std::ios::binary);
	outfile.write(reinterpret_cast<char*>(rom.data()), rom.size());
	outfile.close();


	return 0;
}


int main(int argc, char** argv)
{
	try
	{
		TCLAP::CmdLine cmd("Utility for converting 3D room tilemaps between compressed binary and CSV.\n"
			"Part of the landstalker_tools set: github.com/lordmir/landstalker_tools\n"
			" - Written by LordMir, June 2020",
			' ', "0.1");

		std::vector<std::string> formats{ "csv","map","lz77","rle","cbs" };
		TCLAP::ValuesConstraint<std::string> allowedVals(formats);

		TCLAP::UnlabeledValueArg<std::string> cmpFile("cmpfile", "The CMP (compressed map) file to read/write", true, "", "cmp_filename");
		TCLAP::ValueArg<std::string> romTest("t", "romtest", "Run a map compression/decompression test on the provided US ROM.\n", false, "", "rom_filename");
		TCLAP::ValueArg<std::string> bgFile("b", "background", "The CSV file containing the background layer data to read/write.\n", true, "", "bg_filename");
		TCLAP::ValueArg<std::string> fgFile("g", "foreground", "The CSV file containing the foreground layer data to read/write.\n", true, "", "fg_filename");
		TCLAP::ValueArg<std::string> hmFile("m", "heightmap", "The CSV file containing the heightmap data to read/write.\n", true, "", "hm_filename");
		TCLAP::SwitchArg compress("c", "compress", "Compresses the three provided CSV files into a single CMP file", false);
		TCLAP::SwitchArg decompress("d", "decompress", "Decompresses the provided CMP file into three CSV files (foreground, background, heightmap)", false);
		TCLAP::SwitchArg force("f", "force", "Force overwrite if file already exists and no offset has been set", false);
		TCLAP::ValueArg<uint32_t> inOffset("", "inoffset", "Offset into the input file to start reading data, useful if working with the raw ROM", false, 0, "offset");
		TCLAP::ValueArg<uint32_t> outOffset("", "outoffset", "Offset into the output file to start writing data, useful if working with the raw ROM.\n"
			"**WARNING** This program will not make any attempt to rearrange data in the ROM. If the compressed "
			"size is greater than expected, then data could be overwritten!", false, 0, "offset");
		cmd.add(force);
		cmd.add(cmpFile);
		cmd.add(romTest);
		cmd.add(bgFile);
		cmd.add(fgFile);
		cmd.add(hmFile);
		cmd.xorAdd(compress, decompress);
		cmd.add(inOffset);
		cmd.add(outOffset);
		cmd.parse(argc, argv);

		std::vector<uint8_t> cmp;

		if (romTest.isSet())
		{
			return romtest(romTest.getValue(), cmpFile.getValue());
		}

		if (compress.isSet() && inOffset.isSet())
		{
			throw std::runtime_error("Error: Unable to read CSV file from offset");
		}
		else if (decompress.isSet() && outOffset.isSet())
		{
			throw std::runtime_error("Error: Unable to write CSV file to offset");
		}

		// First, check the CMP file and cache if desired
		std::ifstream ifs(cmpFile.getValue(), std::ios::binary);
		if (ifs.good() == false)
		{
			if (decompress.isSet() == true)
			{
				std::ostringstream msg;
				msg << "Unable to open file \"" << cmpFile.getValue() << "\" for reading.";
				throw std::runtime_error(msg.str());
			}
			else if (outOffset.isSet() == true)
			{
				std::ostringstream msg;
				msg << "Unable to write to offset as file \"" << cmpFile.getValue() << "\" can't be opened.";
				throw std::runtime_error(msg.str());
			}
		}
		else
		{
			if (decompress.isSet() == true || outOffset.isSet() == true)
			{
				ifs.unsetf(std::ios::skipws);

				ifs.seekg(0, std::ios::end);
				std::streampos filesize = ifs.tellg();
				ifs.seekg(0, std::ios::beg);

				if (filesize > 0 && inOffset.getValue() >= filesize)
				{
					std::ostringstream msg;
					msg << "Provided offset " << inOffset.getValue() << " is greater than the size of the file \"" << cmpFile.getValue() << "\" (" << filesize << " bytes).";
					throw std::runtime_error(msg.str());
				}

				cmp.reserve(static_cast<unsigned int>(filesize));
				cmp.insert(cmp.begin(), std::istream_iterator<uint8_t>(ifs), std::istream_iterator<uint8_t>());
			}
			else if (force.isSet() == false)
			{
				std::ostringstream msg;
				msg << "Unable to write to file \"" << cmpFile.getValue() << "\" as it already exists. Try running the command again with the -f flag.";
				throw std::runtime_error(msg.str());
			}
		}
		ifs.close();

		// Next, check our CSV files
		std::fstream foreground(openCSVFile(fgFile.getValue(), decompress.isSet(), force.isSet()));
		std::fstream background(openCSVFile(bgFile.getValue(), decompress.isSet(), force.isSet()));
		std::fstream heightmap(openCSVFile(hmFile.getValue(), decompress.isSet(), force.isSet()));

		// Now, compress/decompress

		std::vector<uint8_t> outbuffer;
		if (decompress.isSet() == true)
		{
			RoomTilemap rt;
			Tilemap3DCmp::Decode(cmp.data(), rt);
			std::cout << "Map: " << (int)rt.width << "x" << (int)rt.height << " " << (int)rt.left << "," << (int)rt.top
				<< " Heightmap: " << (int)rt.hmwidth << "x" << (int)rt.hmheight << std::endl;
			ConvertMapToCSV(rt, background, foreground, heightmap);
		}
		else
		{
			RoomTilemap rt = GetMapFromCSV(background, foreground, heightmap);

			outbuffer.resize(65536);
			size_t esize = Tilemap3DCmp::Encode(rt, outbuffer.data(), outbuffer.size());
			outbuffer.resize(esize);
		}

		// Finally, write-out CMP if needed
		if(compress.isSet())
		if (cmp.size() < outOffset.getValue() + outbuffer.size())
		{
			cmp.resize(outOffset.getValue() + outbuffer.size());
		}
		std::copy(outbuffer.begin(), outbuffer.begin() + outbuffer.size(), cmp.begin() + outOffset.getValue());

		std::ofstream ofs(cmpFile.getValue(), std::ios::binary | std::ios::trunc);
		if (ofs.good() == false)
		{
			std::ostringstream msg;
			msg << "Unable to open output file \"" << cmpFile.getValue() << "\" for writing.";
			throw std::runtime_error(msg.str());
		}
		else
		{
			ofs.write(reinterpret_cast<const char*>(cmp.data()), cmp.size());
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
