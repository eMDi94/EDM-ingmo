#include <iostream>
#include <fstream>
#include <iterator>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <string>

using namespace std;

/*
 * Error handling
 */

void syntax() {
	cerr << "Usage: snappy_decomp <input filename> <output filename>\n";
	exit(EXIT_FAILURE);
}

void error(string&& error_message) {
	cerr << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

/*
 * End of error handling
 */


/*
 * Reading varint
 */

uint64_t read_size(istream& is) {
	uint64_t size = 0;
	size_t count = 0;
	
	while (true) {
		uint8_t val = 0;
		is.read(reinterpret_cast<char*>(&val), 1);
		const uint8_t most_s_bit = val & 0x80;
		val = val & 0x7F;
		size = size | (uint64_t(val) << (count++ * 8));
		if (most_s_bit == 0)
			break;
	}

	return size;
}


/*
 * End of varint
 */

/*
 * Read Literal
 */

void read_literal(istream& is, ostream& os, const uint8_t val) {
	size_t len = 0;
	if (val < 60)
		len = val + 1;
	else {
		const size_t v = val - 59;
		is.read(reinterpret_cast<char*>(&len), v);
		++len;
	}
	copy_n(istream_iterator<uint8_t>(is), len, ostream_iterator<uint8_t>(os));
}

/*
 * End of reading literal
 */

template<typename T>
void read_copy(istream& is, fstream& fs, const uint8_t val, const uint8_t length_mask, const uint8_t offset_mask = 0) {
	size_t len = size_t(val & length_mask);
	size_t offset = size_t(val & offset_mask) << 5;
	T bytes = 2;
	is.read(reinterpret_cast<char*>(&bytes), sizeof(T));
	offset += bytes;

	fs.seekg(-int64_t(offset), ios::cur);
	if (sizeof(T) == 1)
		len += 4;
	else
		len += 1;
	vector<uint8_t> buffer;
	if (size_t(len) > offset) {
		copy_n(istream_iterator<uint8_t>(fs), offset, back_inserter(buffer));
		for (size_t i = 0; i < len - offset; ++i)
			buffer.emplace_back(buffer[i]);
	}
	else
		copy_n(istream_iterator<uint8_t>(fs), len, back_inserter(buffer));

	fs.seekp(0, ios::end);
	copy(begin(buffer), end(buffer), ostream_iterator<uint8_t>(fs));
}

/*
 * Snappy main function
 */

void snappy_decompress(const string& input_filename, const string& output_filename) {
	ifstream is(input_filename, ios::binary);
	is.unsetf(ios::skipws);
	if (!is)
		error("Input file not found.");
	
	fstream fs;
	fs.open(output_filename, ios::out);
	fs.close();
	fs.open(output_filename, ios::in | ios::out | ios::binary);
	fs.unsetf(ios::skipws);
	if (!fs)
		error("Problems with output file.");

	const uint64_t file_len = read_size(is);
	cout << "Size of the uncompressed file: " << file_len << "\n";

	while (true) {
		uint8_t byte;
		is.read(reinterpret_cast<char*>(&byte), 1);
		if (is.eof())
			break;
		const uint8_t v = byte & 0x03;
		byte = byte >> 2;
		switch (v) {
		case 0:
			read_literal(is, fs, byte);
			break;
		case 1:
			read_copy<uint8_t>(is, fs, byte, 0x07, 0x38);
			break;
		case 2:
			read_copy<uint16_t>(is, fs, byte, 0x3F);
			break;
		case 3:
			read_copy<size_t>(is, fs, byte, 0x3F);
			break;
		default:
			// This could never happen.
			break;
		}
	}
}

/*
 * End of snappy main function
 */

int main(const int argc, char **argv) {
	if (argc != 3)
		syntax();

	const string input_filename(argv[1]);
	const string output_filename(argv[2]);

	snappy_decompress(input_filename, output_filename);
	cout << "Done!!!\n";

	return EXIT_SUCCESS;
}