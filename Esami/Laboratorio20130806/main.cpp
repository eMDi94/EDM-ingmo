#include "core.h"
#include "pgm.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdint>
#include <iterator>
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>

#define IMG_WIDTH_TAG 256
#define IMG_LENGTH_TAG 257
#define STRIP_OFFSETS_TAG 273

using namespace std;
using namespace core;
using namespace pgm;


struct tag_pair {
	string tag_name;
	bool fit_values;

	tag_pair(string tag_name_, bool fit_values_) : tag_name(move(tag_name_)), fit_values(fit_values_) {}
};

const unordered_map<uint16_t, tag_pair> tags_table = {
	{262, {"PhotometricInterpretation", true}},
	{259, {"Compression", true}},
	{257, {"ImageLength", true}},
	{256, {"ImageWidth", true}},
	{296, {"ResolutionUnit", true}},
	{282, {"XResolution", false}},
	{283, {"YResolution", false}},
	{278, {"RowsPerStrip", true}},
	{273, {"StripOffset", true}},
	{279, {"StripByteCounts", true}},
	{258, {"BitsPerSample", true}}
};

void syntax() {
	cerr << "Usage: tif2pgm <input_filename>.tif <output_file>.txt\n";
	exit(EXIT_FAILURE);
}

void error(string error_message) {
	cerr << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

inline bool check_extension(const string& filename, string extension) {
	return filename.substr(filename.size() - extension.size()) == extension;
}

struct ifd_entry {
	uint16_t tag, field_type;
	size_t count, value_offset;
};

inline size_t read_header(istream& is) {
	char buffer[8];
	is.read(buffer, 8);
	uint16_t b_order = *(reinterpret_cast<uint16_t*>(buffer));
	if (b_order != uint16_t(0x4949))
		error("Sorry, this decoder only accept little endian tif images.");
	cout << "Byte order: " << string(reinterpret_cast<const char*>(&b_order), 2) << "\n";
	uint16_t tif_id = *(reinterpret_cast<uint16_t*>(buffer + 2));
	if (tif_id != 42)
		error("This is not a tif file.");
	cout << "Tif_Id: " << tif_id << "\n";
	size_t first_ifd = *(reinterpret_cast<size_t*>(buffer + 4));
	cout << "First IFD offset: " << first_ifd << "\n";
	return first_ifd;
}

inline vector<ifd_entry> read_ifd_entry(istream& is, size_t ifd_offset) {
	is.seekg(ifd_offset, ios::beg);

	uint16_t count;
	is.read(reinterpret_cast<char*>(&count), 2);
	vector<ifd_entry> entries(count);
	for (uint16_t i = 0; i < count; ++i) {
		ifd_entry entry;
		is.read(reinterpret_cast<char*>(&entry), sizeof(ifd_entry));
		entries[i] = move(entry);
	}

	return entries;
}

inline void dump_compression(size_t compression) {
	cout << "Short Value: ";
	switch (compression) {
	case 1:
		cout << "No Compression\n";
		break;
	case 2:
		cout << "CCIT Group 3\n";
		break;
	case 32773:
		cout << "Packbits Compression\n";
		break;
	default:
		error("Compression method not recognized.");
	}
}

inline void dump_photometric_interpretation(size_t photometric_interpretation) {
	cout << "Short Value: ";
	switch (photometric_interpretation) {
	case 0:
		cout << "WhiteIsZero\n";
		break;
	case 1:
		cout << "BlackIsZero\n";
		break;
	default:
		error("Photometric Interpretation not recognized.");
	}
}

inline void dump_resolution_unit(size_t resolution_unit) {
	cout << "Short Value: ";
	switch (resolution_unit) {
	case 1:
		cout << "No absolute unit of measurement\n";
		break;
	case 2:
		cout << "Inch\n";
		break;
	case 3:
		cout << "Centimeter\n";
		break;
	default:
		error("Resolution Unit not recognized.");
	}
}

inline void dump_rational(istream& is, size_t offset, bool sign = false) {
	size_t current_pos = is.tellg();
	is.seekg(offset, ios::beg);

	if (sign) {
		int32_t num, den;
		is.read(reinterpret_cast<char*>(&num), 4);
		is.read(reinterpret_cast<char*>(&den), 4);
		cout << num << "/" << den << "\n";
	}
	else {
		size_t num, den;
		is.read(reinterpret_cast<char*>(&num), 4);
		is.read(reinterpret_cast<char*>(&den), 4);
		cout << num << "/" << den << "\n";
	}


	is.seekg(current_pos, ios::beg);
}

inline void dump_ascii(istream& is, size_t offset, size_t count) {
	size_t current_position = is.tellg();
	is.seekg(offset, ios::beg);

	string str;
	copy_n(istream_iterator<char>(is), count, back_inserter(str));

	is.seekg(current_position, ios::beg);
}

inline void dump_double(istream& is, size_t offset) {
	size_t current_position = is.tellg();
	is.seekg(offset, ios::beg);

	double d;
	is.read(reinterpret_cast<char*>(&d), 8);
	cout << d << "\n";

	is.seekg(current_position, ios::beg);
}

inline void dump_field_type_and_data(istream& is, const ifd_entry& entry) {
	switch (entry.field_type) {
	case 1:
		cout << "Byte Value: " << entry.value_offset << "\n";
		break;
	case 2:
		cout << "ASCII Value: ";
		dump_ascii(is, entry.value_offset, entry.count);
		break;
	case 3:
		cout << "Short Value: " << entry.value_offset << "\n";
		break;
	case 4:
		cout << "Long Value: " << entry.value_offset << "\n";
		break;
	case 5:
		cout << "Rational Value: ";
		dump_rational(is, entry.value_offset);
		break;
	case 6:
		cout << "SByte Value: " << static_cast<int16_t>(entry.value_offset & 0xFF) << "\n";
		break;
	case 7:
		cout << "Undefined\n";
		break;
	case 8:
		cout << "SShort Value: " << static_cast<int16_t>(entry.value_offset & 0xFFFF) << "\n";
		break;
	case 9:
		cout << "SLong Value: " << static_cast<int32_t>(entry.value_offset) << "\n";
		break;
	case 10:
		cout << "SRational Value: ";
		dump_rational(is, entry.value_offset, true);
		break;
	case 11:
		cout << "Float Value: " << static_cast<float>(entry.value_offset) << "\n";
		break;
	case 12:
		cout << "Dobel Value: ";
		dump_double(is, entry.value_offset);
		break;
	default:
		error("Type not recognized.");
	}
}

inline void dump_entry(const ifd_entry& entry, istream& is) {
	auto it = tags_table.find(entry.tag);
	if (it == end(tags_table))
		return;

	const tag_pair& tp = it->second;
	if (tp.fit_values) {
		cout << tp.tag_name << " ";
		switch (entry.tag) {
		case 259:
			dump_compression(entry.value_offset);
			break;
		case 296:
			dump_resolution_unit(entry.value_offset);
			break;
		case 262:
			dump_photometric_interpretation(entry.value_offset);
			break;
		default:
			dump_field_type_and_data(is, entry);
			break;
		}
	}
}

inline mat<uint8_t> read_image(istream& is, size_t ifd_offset) {
	vector<ifd_entry> entries = read_ifd_entry(is, ifd_offset);

	size_t rows, cols, offset;
	for (const auto& entry : entries) {
		dump_entry(entry, is);
		switch (entry.tag) {
		case IMG_LENGTH_TAG:
			rows = entry.value_offset;
			break;
		case IMG_WIDTH_TAG:
			cols = entry.value_offset;
			break;
		case STRIP_OFFSETS_TAG:
			offset = entry.value_offset;
			break;
		default:
			break;
		}
	}

	mat<uint8_t> img(rows, cols);
	is.seekg(offset, ios::beg);
	is.read(reinterpret_cast<char*>(img.data()), rows*cols);

	return img;
}

void tif2pgm(const string& input_filename, const string& output_filename) {
	if (!check_extension(input_filename, ".tif"))
		error("Input file must be a .tif file.");
	if (!check_extension(output_filename, ".pgm"))
		error("Output file must be a .pgm file.");

	ifstream is(input_filename, ios::binary);
	if (!is)
		error("Cannot open input file.");

	size_t first_ifd = read_header(is);
	mat<uint8_t> img = read_image(is, first_ifd);
	ofstream os(output_filename, ios::binary);
	if (!os)
		error("Cannot open output file for saving the image.");
	if (!save_pgm(os, img))
		error("Cannot save output image.");
}

int main(int argc, char **argv) {
	if (argc != 3)
		syntax();

	string input(argv[1]);
	string output(argv[2]);
	tif2pgm(input, output);
	cout << "Done!!\n";

	return EXIT_SUCCESS;
}