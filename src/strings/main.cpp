#include <iostream>
#include <stdexcept>
#include <fstream>
#include <vector>
#include <cstdint>
#include <memory>
#include <iterator>
#include <functional>
#include <sys/stat.h>
#include <codecvt>
#include <locale>

#include <LSString.h>
#include <IntroString.h>
#include <EndCreditString.h>
#include <HuffmanString.h>
#include <HuffmanTrees.h>
#include <Charset.h>
#define TCLAP_SETBASE_ZERO 1
#include <tclap/CmdLine.h>

std::shared_ptr<HuffmanTrees> huffman_trees;

bool fileExists(const std::string& filename)
{
	struct stat buffer;
	return (stat(filename.c_str(), &buffer) == 0);
}

void CacheBinaryFile(std::string filename, std::vector<uint8_t>& data, size_t file_offset)
{
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

		if (filesize > 0 && file_offset >= filesize)
		{
			std::ostringstream msg;
			msg << "Provided offset " << file_offset << " is greater than the size of the file \"" << filename << "\" (" << file_offset << " bytes).";
			throw std::runtime_error(msg.str());
		}

		data.reserve(data.size() + static_cast<unsigned int>(filesize));
		data.insert(data.end(), std::istream_iterator<uint8_t>(ifs), std::istream_iterator<uint8_t>());
	}
	ifs.close();
}

void CacheBinaryFiles(const std::vector<std::string>& binary_files, std::vector<uint8_t>& data, size_t file_offset)
{
	for (const auto& bfile : binary_files)
	{
		CacheBinaryFile(bfile, data, file_offset);
	}
}

std::fstream OpenOutputFile(std::string filename, bool force)
{
	std::fstream decodedfs;
	if (force == false && fileExists(filename))
	{
		std::ostringstream msg;
		msg << "Unable to write to file \"" << filename << "\" as it already exists. Try running the command again with the -f flag.";
		throw std::runtime_error(msg.str());
	}
	decodedfs.open(filename, std::ios::out | std::ios::trunc);
	if (decodedfs.good() == false)
	{
		std::ostringstream msg;
		msg << "Unable to open file \"" << filename << "\" for writing.";
		throw std::runtime_error(msg.str());
	}
	return decodedfs;
}

void ParseDecodedFile(const std::string& filename, const std::string& format, std::vector<std::shared_ptr<LSString>>& decoded)
{
	std::ifstream decodedfs;
	std::wstring_convert<std::codecvt_utf8<LSString::StringType::value_type>> utf8_conv;
	decodedfs.open(filename, std::ios::in);
	if (decodedfs.good() == false)
	{
		std::ostringstream msg;
		msg << "Unable to open file \"" << filename << "\" for reading.";
		throw std::runtime_error(msg.str());
	}
	else
	{
		std::string line;
		std::wstring wline;
		if (format == "intro" || format == "ending")
		{
			std::getline(decodedfs, line); // Discard header row
		}
		while (decodedfs.eof() == false)
		{
			std::getline(decodedfs, line);
			wline = utf8_conv.from_bytes(line);
			if (line.empty() == false)
			{
				if (format == "names")
				{
					decoded.push_back(std::make_shared<LSString>(wline));
				}
				else if (format == "intro")
				{
					decoded.push_back(std::make_shared<IntroString>(wline));
				}
				else if (format == "ending")
				{
					decoded.push_back(std::make_shared<EndCreditString>(wline));
				}
				else if (format == "main")
				{
					decoded.push_back(std::make_shared<HuffmanString>(wline, huffman_trees));
				}
			}
		}
	}
}

void ParseEncodedBuffer(const std::vector<uint8_t>& encoded, const std::string& format, std::vector<std::shared_ptr<LSString>>& decoded)
{
	size_t offset = 0;
	while (offset < encoded.size())
	{
		if (format == "names")
		{
			decoded.push_back(std::make_shared<LSString>());
		}
		else if (format == "intro")
		{
			decoded.push_back(std::make_shared<IntroString>());
		}
		else if (format == "ending")
		{
			decoded.push_back(std::make_shared<EndCreditString>());
		}
		else if (format == "main")
		{
			decoded.push_back(std::make_shared<HuffmanString>(huffman_trees));
		}
		offset += decoded.back()->Decode(encoded.data() + offset, encoded.size() - offset);
	}
}

void WriteDecodedData(std::string filename, bool force, std::vector<std::shared_ptr<LSString>>& decoded)
{
	std::fstream decodedfs = OpenOutputFile(filename, force);
	std::wstring_convert<std::codecvt_utf8<LSString::StringType::value_type>> utf8_conv;
	if (decoded.front()->GetHeaderRow().length() > 0)
	{
		decodedfs << utf8_conv.to_bytes(decoded.front()->GetHeaderRow()) << std::endl;
	}
	for (const auto& line : decoded)
	{
		decodedfs << utf8_conv.to_bytes(line->Serialise()) << std::endl;
	}
	decodedfs.close();
}

void EncodeData(const std::vector<std::shared_ptr<LSString>>& decoded, std::string format, std::vector<std::vector<uint8_t>>& encoded)
{
	size_t lines = 0;
	size_t split = 0;
	if (format == "intro")
	{
		split = 1;
	}
	else if (format == "main")
	{
		std::cout << "Compressing text strings..." << std::endl;
		split = 256;
	}

	encoded.push_back(std::vector<uint8_t>(65536));
	size_t offset = 0;
	for (const auto p : decoded)
	{
		lines++;
		if (offset >= encoded.back().size())
		{
			throw std::runtime_error("Out of buffer memory!");
		}
		offset += p->Encode(encoded.back().data() + offset, encoded.back().size() - offset);
		if (split > 0 && lines % split == 0)
		{
			encoded.back().resize(offset);
			encoded.push_back(std::vector<uint8_t>(65536));
			offset = 0;
		}
		if (lines % 100 == 0)
		{
			std::cout << ".";
		}
	}
	encoded.back().resize(offset);
	while (encoded.back().empty())
	{
		encoded.pop_back();
	}
}

std::ofstream OpenBinaryFileForWriting(const std::string& filename, bool force, size_t offset)
{
	if (offset == 0 && force == false && fileExists(filename))
	{
		std::ostringstream msg;
		msg << "Unable to write to file \"" << filename << "\" as it already exists. Try running the command again with the -f flag.";
		throw std::runtime_error(msg.str());
	}
	std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
	if (ofs.good() == false)
	{
		std::ostringstream msg;
		msg << "Unable to open output file \"" << filename << "\" for writing.";
		throw std::runtime_error(msg.str());
	}
	return ofs;
}

void WriteBinaryFile(const std::string& filename, bool force, const std::vector<uint8_t>& data, size_t offset)
{
	std::ofstream ofs(OpenBinaryFileForWriting(filename, force, offset));
	if (offset > 0)
	{
		std::vector<uint8_t> buffer;
		CacheBinaryFile(filename, buffer, 0);
		buffer.resize(data.size() + offset);
		std::copy(data.begin(), data.end(), buffer.begin() + offset);
		ofs.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
	}
	else
	{
		ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
	}
}

void WriteEncodedData(const std::string& filename, bool use_pattern, bool force, const std::vector<std::vector<uint8_t>>& encoded, size_t offset, std::string ext = ".bin")
{
	size_t total = 0;
	size_t file = 1;
	std::string outfile;
	std::ofstream ofs;

	if (offset > 0)
	{
		std::vector<uint8_t> buffer;
		CacheBinaryFile(filename, buffer, 0);
		for (const auto& enc : encoded)
		{
			if (enc.size() + offset > buffer.size())
			{
				buffer.resize(enc.size() + offset);
			}
			std::copy(enc.begin(), enc.end(), buffer.begin() + offset);
			offset += enc.size();
		}
		WriteBinaryFile(filename, true, buffer, 0);
	}
	else
	{
		for (const auto& enc : encoded)
		{
			if (use_pattern == true)
			{
				std::ostringstream ss;
				if (ofs.is_open())
				{
					file++;
					ofs.close();
				}
				ss << filename << std::setw(2) << std::setfill('0') << file << ext;
				outfile = ss.str();
				ofs = OpenBinaryFileForWriting(outfile, force, 0);
			}
			else
			{
				if (ofs.is_open() == false)
				{
					ofs = OpenBinaryFileForWriting(filename, force, 0);
				}
			}
			ofs.write(reinterpret_cast<const char*>(enc.data()), enc.size());
		}
	}
}

int main(int argc, char** argv)
{
	try
	{
		TCLAP::CmdLine cmd("Utility for converting strings between different formats.\n"
			"Part of the landstalker_tools set: github.com/lordmir/landstalker_tools\n"
			" - Written by LordMir, June 2020",
			' ', "0.1");

		std::vector<std::string> formats{ "main","intro","ending","names" };
		std::vector<std::string> languages{ "en","jp","fr","de" };
		TCLAP::ValuesConstraint<std::string> allowedFormats(formats);
		TCLAP::ValuesConstraint<std::string> allowedLangs(languages);

		TCLAP::UnlabeledMultiArg<std::string> files("filenames", "The files to convert. If extracting binary data, then this must be a list of one "
			                                                     "or more binary input files, followed by the output text filename. If encoding "
			                                                     "ASCII data, then this must be a single ASCII input file followed by an optional "
			                                                     "binary output filename.", true, "", "filenames");
		TCLAP::SwitchArg outputPrefix("p", "output_file_prefix", "Treats the output file name as a file pattern to be used when decoding source data that should be "
		                                                         "output to multiple files. E.g. if the output file pattern is set to './example', then data will be "
			                                                     "output to ./example01.bin, ./example02.bin, etc.", false);
		TCLAP::ValueArg<std::string> hTable("u", "huffman_table", "The file containing the Huffman compression tables.\n", false, "", "huffman_table");
		TCLAP::ValueArg<std::string> hOffsetTable("t", "huffman_offset_table", "The file containing the Huffman table offsets.\n", false, "", "huffman_offsets");
		TCLAP::ValueArg<uint32_t> hTableOff("U", "huffman_table_offset", "The offset in ROM to the Huffman compression tables.\n", false, 0, "offset");
		TCLAP::ValueArg<uint32_t> hOffsetTableOff("T", "huffman_offset_table_offset", "The offset in ROM to the Huffman table offsets.\n", false, 0, "offset");
		TCLAP::ValueArg<std::string> format("r", "format", "The string format to use.\n", true, "names", &allowedFormats);
		TCLAP::ValueArg<std::string> language("l", "language", "The string language to use.\n", true, "en", &allowedLangs);
		TCLAP::SwitchArg recalcHuffman("x", "recalc_huffman", "Recalculates the huffman tables and outputs the result to the files identified with the -u and -t flags.", false);
		TCLAP::SwitchArg compress("c", "convert", "Converts the provided ASCII string table into binary data", false);
		TCLAP::SwitchArg decompress("e", "extract", "Extracts the provided binary string data into an ASCII string table", false);
		TCLAP::SwitchArg force("f", "force", "Force overwrite if file already exists and no offset has been set", false);
		TCLAP::ValueArg<uint32_t> inOffset("i", "inoffset", "Offset into the input file to start reading data, useful if working with the raw ROM", false, 0, "offset");
		TCLAP::ValueArg<uint32_t> outOffset("o", "outoffset", "Offset into the output file to start writing data, useful if working with the raw ROM.\n"
			"**WARNING** This program will not make any attempt to rearrange data in the ROM. If the compressed "
			"size is greater than expected, then data could be overwritten!", false, 0, "offset");
		cmd.add(force);
		cmd.add(format);
		cmd.add(recalcHuffman);
		cmd.add(hTable);
		cmd.add(hTableOff);
		cmd.add(hOffsetTable);
		cmd.add(hOffsetTableOff);
		cmd.xorAdd(compress, decompress);
		cmd.add(inOffset);
		cmd.add(outOffset);
		cmd.add(outputPrefix);
		cmd.add(files);
		cmd.parse(argc, argv);
		std::string inFile = "";
		std::string outFile = "";
		bool use_pattern = false;
		std::vector<std::string> binary_files;
		size_t total_files = files.end() - files.begin();
		if (decompress.isSet())
		{
			if (total_files < 2)
			{
				throw std::runtime_error("Expected 2 or more files when in decode mode.");
			}
			outFile = *std::prev(files.end());
			std::copy(files.begin(), std::prev(files.end()), std::back_inserter(binary_files));
		}
		else
		{
			if (total_files > 2)
			{
				throw std::runtime_error("Expected only 2 files when in encode mode.");
			}
			else if (total_files == 2)
			{
				if (outOffset.isSet() == true && outputPrefix.isSet() == true)
				{
					throw std::runtime_error("You cannot specify both an output prefix and an output offset.");
				}
				else if (outputPrefix.isSet() == true)
				{
					use_pattern = true;
				}
				outFile = *std::prev(files.end());

			}
			else // total_files < 2
			{
				throw std::runtime_error("Expected only 2 files when in encode mode.");
			}
			inFile = *files.begin();
		}

		std::vector<uint8_t> encoded;
		std::vector<std::shared_ptr<LSString>> decoded;
		std::vector<std::vector<uint8_t>> outbuffer;
		std::string hufftablefile;
		std::string huffofffile;

		if (hOffsetTableOff.isSet())
		{
			huffofffile = outFile;
		}
		else if (hOffsetTable.isSet())
		{
			huffofffile = hOffsetTable.getValue();
		}

		if (hTableOff.isSet())
		{
			hufftablefile = outFile;
		}
		else if (hTable.isSet())
		{
			hufftablefile = hTable.getValue();
		}
		
		if (hufftablefile.empty() == false && huffofffile.empty() == false &&
		    (decompress.isSet() == true || recalcHuffman.isSet() == false))
		{
			std::vector<uint8_t> huffoff;
			std::vector<uint8_t> hufftrs;
			CacheBinaryFile(huffofffile, huffoff, hOffsetTableOff.getValue());
			CacheBinaryFile(hufftablefile, hufftrs, hTableOff.getValue());
			huffman_trees = std::make_shared<HuffmanTrees>(huffoff.data(), huffoff.size(), hufftrs.data(), hufftrs.size(), huffoff.size() / 2);
		}

		if (decompress.isSet())
		{
			
			CacheBinaryFiles(binary_files, encoded, outOffset.getValue());
			ParseEncodedBuffer(encoded, format.getValue(), decoded);
			WriteDecodedData(outFile, force.isSet(), decoded);
		}
		else
		{
			if (recalcHuffman.isSet())
			{
				huffman_trees = std::make_shared<HuffmanTrees>();
			}
			ParseDecodedFile(inFile, format.getValue(), decoded);
			if (recalcHuffman.isSet())
			{
				if (huffofffile.empty() == false && hufftablefile.empty() == false)
				{
					std::vector<uint8_t> huff_offsets;
					std::vector<uint8_t> huff_trees;
					huffman_trees->RecalculateTrees(decoded);
					huffman_trees->EncodeTrees(huff_offsets, huff_trees);
					WriteBinaryFile(huffofffile, force.getValue(), huff_offsets, hOffsetTableOff.getValue());
					WriteBinaryFile(hufftablefile, force.getValue(), huff_trees, hTableOff.getValue());
				}
				else
				{
					throw std::runtime_error("Unable to write out recalculated trees: no filenames given");
				}
			}
			EncodeData(decoded, format.getValue(), outbuffer);
			WriteEncodedData(outFile, use_pattern, force.isSet(), outbuffer, outOffset.getValue(), decoded.back()->GetEncodedFileExt());
		}

		std::cout << "Done!" << std::endl;

		return 0;
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
}