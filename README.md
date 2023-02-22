# landstalker_tools
A collection of tools to allow users to edit the compressed/encoded assets found in Landstalker and other games.

## Installation
### Requirements
* CMake version 3.13 or greater is required: https://cmake.org/install/
* A C++ compiler that supports C++17 (gcc 7.3 or MSVC2019)

## Linux
1. Run the `configure.sh` script
2. Run the `build.sh` script to build the binaries
3. (If desired), run the `install.sh` script to install the binaries to the system

## Windows
1. Run the `configure.bat` script
2. Run the `build.bat` script to build the binaries
3. (If desired), run the `install.bat` script to install the binaries to the system

## Tools
### LZ77
A simple command-line tool to compress and decompress LZ77 data.

Usage:

`lz77  {-d|-c} [-o <offset>] [-i <offset>] [-f] [--] [--version] [-h] <in_filename> <out_filename>`

Where:

- -d,  --decompress
     (OR required)  Decompress [input_file], and write result to
     [output_file]
         -- OR --
-  -c,  --compress
     (OR required)  Compress [input_file] and write result to [output_file]


-  -o <offset>,  --outoffset <offset>
     Offset into the output file to start writing data, useful if working
     with the raw ROM.

     **WARNING** This program will not make any attempt to rearrange data
     in the ROM. If the compressed size is greater than expected, then data
     could be overwritten!

-  -i <offset>,  --inoffset <offset>
     Offset into the input file to start reading data, useful if working
     with the raw ROM

-  -f,  --force
     Force overwrite if file already exists and no offset has been set

-  --,  --ignore_rest
     Ignores the rest of the labeled arguments following this flag.

-  --version
     Displays version information and exits.

-  -h,  --help
     Displays usage information and exits.

-  <in_filename>
     (required)  The input file (.lz77/.bin)

-  <out_filename>
     (required)  The output file (.lz77/.bin)}

# Building
## Windows - Visual Studio Community 2019

1. Open the `vs2019\landstalker_tools.sln` file in Visual Studio 2019.
2. Make sure you have selected the appropriate build (e.g. Release x64) and go to Build->Build Solution (Ctrl+Shift+B)
3. The built binaries should be output to the vs2019\Release or vs2019\Debug folders.

## Linux - gcc

1. Change into the top-level directory for the project and run `make`.
