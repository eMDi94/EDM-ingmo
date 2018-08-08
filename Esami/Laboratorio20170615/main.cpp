#include "core.h"
#include "ppm.h"
#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <numeric>

using namespace std;
using namespace core;
using namespace ppm;

array<char, 85> z85_values = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
	'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
	'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D',
	'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', '.', '-', ':', '+', '=', '^', '!', '/',
	'*', '?', '&', '<', '>', '(', ')', '[', ']', '{',
	'}', '@', '%', '$', '#'
};

void syntax() {
	cerr << "Usage: z85rot [c|d] <N> <input_filename> <output_filename>\n";
	exit(EXIT_FAILURE);
}

void error(string error_message) {
	cerr << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

inline bool check_extension(const string& filename, const string extension) {
	return filename.substr(filename.size() - extension.size()) == extension;
}

void encode(const size_t N, const string& input_filename, const string& output_filename) {
	if (!check_extension(input_filename, ".ppm"))
		error("Input file must be a .ppm file.");
	if (!check_extension(output_filename, ".z85r"))
		error("Output file must be a .z85r file.");
	
	ifstream is(input_filename, ios::binary);
	if (!is)
		error("Cannot open input file.");
	mat<vec3b> img;
	if (!load_ppm(is, img))
		error("Cannot load the input image.");

	ofstream os(output_filename, ios::binary);
	if (!os)
		error("Cannot open the output file.");
	os << img.width() << "," << img.height() << ",";
	const uint8_t *data = reinterpret_cast<const uint8_t*>(img.data());
	size_t tot_size = img.height() * img.width() * 3;
	for (size_t i = 0; i < tot_size;) {
		size_t k = 0;
		uint32_t value = 0;
		while (k++ < 4 && i < tot_size) {
			value = value << 8;
			uint32_t tmp = uint32_t(data[i++]);
			value |= tmp;
		}

		if (k < 4)
			while (k++ <= 4)
				value = (value << 8) | 0;

		array<uint32_t, 5> indexes;
		for (size_t j = 5; j > 0; --j) {
			uint32_t mod = value % 85;
			indexes[j - 1] = mod;
			value = value / 85;
		}
		for (const auto& v : indexes) {
			char c = z85_values[v];
			os << c;
			rotate(begin(z85_values), end(z85_values) - N, end(z85_values));
		}
	}
}

void decode(const size_t N, const string& input_filename, const string& output_filename) {
	if (!check_extension(input_filename, ".z85r"))
		error("Input file must be a .z85r file.");
	if (!check_extension(output_filename, ".ppm"))
		error("Output file must be a .ppm file.");
	
	ifstream is(input_filename, ios::binary);
	if (!is)
		error("Cannot open input file.");
	size_t width, height;
	char comma1, comma2;
	is >> width >> comma1 >> height >> comma2;
	if (comma1 != ',' || comma2 != ',')
		error("Width must be separated from height by a comma, and height must be seprated from the data by another comma.");
	
	mat<vec3b> img(height, width);
	uint8_t *data = reinterpret_cast<uint8_t*>(img.data());
	size_t tot_size = width * height * 3;
	size_t readed = 0;
	while (is.peek() != EOF && readed < tot_size) {
		char buffer[5];
		is.read(buffer, 5);
		if (!is || is.gcount() != 5)
			error("Input file must be able to give 5 byte every time.");
		uint32_t value = 0;
		for (size_t i = 0; i < 5; ++i) {
			uint32_t pos = find(begin(z85_values), end(z85_values), buffer[i]) - begin(z85_values);
			pos = static_cast<uint32_t>(pow(85, (4 - i)) * pos);
			value += pos;
			rotate(begin(z85_values), end(z85_values) - N, end(z85_values));
		}
		for (size_t i = 0; i < 4 && readed < tot_size; ++i) {
			uint8_t byte = ((value & (0xFF << ((3 - i) * 8))) >> ((3 - i) * 8)) & 0xFF;
			data[readed++] = byte;
		}
	}

	ofstream os(output_filename, ios::binary);
	if (!os)
		error("Cannot open output file.");
	if (!save_ppm(os, img))
		error("Cannot save output image.");
}

int main(int argc, char **argv) {
	if (argc != 5)
		syntax();

	try {
		string command(argv[1]);
		size_t N = stoi(string(argv[2]));
		string input(argv[3]);
		string output(argv[4]);

		if (command == "c")
			encode(N, input, output);
		else
			if (command == "d")
				decode(N, input, output);
			else
				error("Command parameter must be c for encoding and d for decoding.");

		cout << "Done!!\n";

		return EXIT_SUCCESS;
	}
	catch (exception&) {
		error("The second parameter must be convertible to an integer.");
	}
}