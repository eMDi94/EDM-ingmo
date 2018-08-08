#include "bit.h"
#include "core.h"
#include "huffman.h"
#include "ppm.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cstdint>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <iomanip>

using namespace std;
using namespace core;
using namespace bit;
using namespace huffman;
using namespace ppm;

void syntax() {
	cerr << "Usage: octimg <input_filename>.octimg <output_prefix>\n";
	exit(EXIT_FAILURE);
}

void error(string error_message) {
	cerr << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

inline bool check_extension(const string& filename, string extension) {
	return filename.substr(filename.size() - extension.size()) == extension;
}

inline void read_img(canonical_huffman_decoder& decoder, stream_bit_reader& br,
					const size_t height, const size_t width,
					const string& output_prefix, const size_t count) {
	mat<vec3b> img(height, width);

	for (size_t r = 0; r < height; ++r) {
		for (size_t c = 0; c < width; ++c) {
			huffman_code code;
			while (!decoder.contains(code) && !br.eof()) {
				uint8_t bit = br.get<uint8_t>(1);
				code.push_back(bit);
			}
			if (br.eof())
				error("There is no code.");
			const auto& cp = decoder.at(code);
			img(r, c) = { cp.first, cp.second, cp.second };
		}
	}

	stringstream ss;
	ss << output_prefix << setw(3) << setfill('0') << count << ".ppm";
	const auto& str = ss.str();
	ofstream os(str, ios::binary);
	if (!os)
		error("Cannot open output file " + str);
	if (!save_ppm(os, img))
		error("Cannot save image on output file " + str);
}

void decode(const string& input_filename, const string& output_prefix) {
	if (!check_extension(input_filename, ".octimg"))
		error("Input file must be an .octimg file.");
	ifstream is(input_filename, ios::binary);
	if (!is)
		error("Cannot open input file.");
	
	string magic = "OCTIMG";
	copy_n(istream_iterator<char>(is), 6, begin(magic));
	if (magic != "OCTIMG")
		error("octimg file does not start with an OCTIMG string.");
	
	stream_bit_reader br(is);
	size_t width = br.get<size_t>();
	size_t height = br.get<size_t>();
	size_t num_images = br.get<size_t>(10);

	canonical_huffman_decoder decoder(br);
	for (size_t i = 0; i < num_images; ++i)
		read_img(decoder, br, height, width, output_prefix, i + 1);
}

int main(int argc, char **argv) {
	if (argc != 3)
		syntax();

	string input(argv[1]);
	string output(argv[2]);
	decode(input, output);
	cout << "Done!!\n";

	return EXIT_SUCCESS;
}