#include <iostream>
#include <algorithm>
#include "core.h"
#include "multires.h"
#include "pgm.h"
#include <string>
#include <fstream>
#include <cstdlib>
#include <sstream>

using namespace std;
using namespace core;
using namespace pgm;
using namespace multires;

/*
 * Error handling
 */

void syntax() {
	cout << "Usage: multires <option> <input filename> <output filename>\n";
	cout << "<option> could be c for encoding and d for decoding\n";
	exit(EXIT_FAILURE);
}

void error(string&& error_message) {
	cout << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

inline bool check_extension(const string& input_filename, string&& extension) {
	return input_filename.substr(input_filename.size() - extension.size(), extension.size()) == extension;
}

/*
 * End of error handling
 */


/*
 * Encoding
 */

void encode(const string& input_filename, const string& output_filename) {
	if (!check_extension(input_filename, ".pgm"))
		error("Input file must be a .pgm file.");
	ifstream is(input_filename, ios::binary);
	if (!is)
		error("Error on opening the input file.");

	mat<uint8_t> image;
	if (!read_pgm(is, image))
		error("Error during reading the input image.");

	ofstream os(output_filename + ".mlt", ios::binary);
	if (!os)
		error("Error on output file.");

	os << "MULTIRES";
	const size_t height = image.height(), width = image.width();
	os.write(reinterpret_cast<const char*>(&width), sizeof(size_t));
	os.write(reinterpret_cast<const char*>(&height), sizeof(size_t));
	multires_encoder encoder(image);
	for (const auto& level : encoder)
		copy(begin(level), end(level), ostream_iterator<uint8_t>(os));
}

/*
 * End of encoding
 */

/*
 * Decoding
 */

void decode(const string& input_filename, const string& output_filename) {
	if (!check_extension(input_filename, ".mlt"))
		error("Input file must be a .mlt file.");
	ifstream is(input_filename, ios::binary);
	is.unsetf(ios::skipws);
	if (!is)
		error("Error on opening the input file.");
	
	string magic;
	copy_n(istream_iterator<char>(is), 8, back_inserter(magic));
	
	if (magic != "MULTIRES")
		error("Input file is not a multires file.");
	
	size_t width, height;
	is.read(reinterpret_cast<char*>(&width), sizeof(size_t));
	is.read(reinterpret_cast<char*>(&height), sizeof(size_t));

	multires_decoder decoder(height, width, istream_iterator<uint8_t>(is), istream_iterator<uint8_t>());

	for (size_t i = 1; i <= 7; ++i) {
		stringstream ss;
		ss << output_filename << "_" << i << ".pgm";
		ofstream os(ss.str(), ios::binary);
		const mat<uint8_t> r_image = decoder.decode_level(i);
		if (!write_pgm(os, r_image)) {
			ss.clear();
			ss << "Error on writing the image with level " << i << ".";
			error(ss.str());
		}
	}
}

/*
 * End of decoding
 */


int main(const int argc, char **argv) {
	if (argc != 4)
		syntax();

	const string option(argv[1]);
	const string input_filename(argv[2]);
	const string output_filename(argv[3]);

	if (option == "c")
		encode(input_filename, output_filename);
	else
		if (option == "d")
			decode(input_filename, output_filename);
		else
			error("Option not recognized. Allowed options are c for encoding and d for decoding.\n");

	cout << "Done!!\n";

	return EXIT_SUCCESS;
}