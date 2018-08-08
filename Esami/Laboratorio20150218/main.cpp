#include "bit.h"
#include "huffman.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>

using namespace std;
using namespace bit;
using namespace huffman;

void syntax() {
	cerr << "Usage: cmp2xml <input_filename>.cmp <output_filename>.xml\n";
	exit(EXIT_FAILURE);
}

void error(string error_message) {
	cerr << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

inline bool check_extension(const string& filename, string extension) {
	return filename.substr(filename.size() - extension.size()) == extension;
}

inline const string& find_string(stream_bit_reader& br, const canonical_huffman_decoder& decoder) {
	huffman_code code;
	while (!decoder.contains(code)) {
		uint8_t bit = br.get<uint8_t>(1);
		code.insert_back(bit);
	}
	return decoder.at(code);
}

inline void write_tag(stream_bit_reader& br, const canonical_huffman_decoder& decoder, ostream& os, size_t tab = 0) {
	string tab_string(tab, '\t');
	const string& tag = find_string(br, decoder);
	os << tab_string << "<" << tag;
	uint8_t n_attr = br.get<uint8_t>(4);
	for (uint8_t i = 0; i < n_attr; ++i) {
		const string& attr_name = find_string(br, decoder);
		const string& attr_value = find_string(br, decoder);
		os << " " << attr_name << "=\"" << attr_value << "\"";
	}
	uint8_t has_children = br.get<uint8_t>(1);
	if (has_children) {
		os << ">\n";
		uint16_t children_count = br.get<uint16_t>(10);
		for (uint16_t i = 0; i < children_count; ++i)
			write_tag(br, decoder, os, tab + 1);
		os << tab_string << "</" << tag << ">\n";
	}
	else {
		os << ">";
		uint16_t byte_count = br.get<uint16_t>(10);
		for (uint16_t i = 0; i < byte_count; ++i)
			os << br.get<char>(8);
		os << "</" << tag << ">\n";
	}
}

inline void write_xml_file(istream& is, const string& output_filename) {
	stream_bit_reader br(is);
	canonical_huffman_decoder decoder(br);
	ofstream os(output_filename, ios::binary);
	if (!os)
		error("Cannot open output file.");
	os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	write_tag(br, decoder, os);
	if (!os)
		error("Error during writing the .xml file.");
}

void cmp2xml(const string& input_filename, const string& output_filename) {
	if (!check_extension(input_filename, ".cmp"))
		error("Input file must be a .cmp file.");
	if (!check_extension(output_filename, ".xml"))
		error("Output filename must be a .xml file.");
	ifstream is(input_filename, ios::binary);
	if (!is)
		error("Cannot open input file.");
	write_xml_file(is, output_filename);
}

int main(int argc, char **argv) {
	if (argc != 3)
		syntax();

	string input(argv[1]);
	string output(argv[2]);
	cmp2xml(input, output);
	cout << "Done!!\n";

	return EXIT_SUCCESS;
}