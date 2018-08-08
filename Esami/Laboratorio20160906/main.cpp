#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <array>
#include <iterator>
#include <vector>

using namespace std;

void syntax() {
	cerr << "Usage: zip_struct <input_file>.zip\n";
	exit(EXIT_FAILURE);
}

void error(string error_message) {
	cerr << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

inline bool check_extension(const string& filename, string extension) {
	return filename.substr(filename.size() - extension.size()) == extension;
}

inline size_t search_eocd(istream& is) {
	const array<char, 4> eocd_signature = { 0x50, 0x4b, 0x05, 0x06 };
	array<char, 4> search_buffer;
	
	int64_t back = 22;
	is.seekg(0, ios::end);
	int64_t file_size = is.tellg();
	bool eocd_found = false;
	size_t offset = 0;
	string comment = "";
	while (!eocd_found && back <= file_size) {
		is.seekg(-back, ios::end);
		copy_n(istream_iterator<char>(is), 4, begin(search_buffer));
		if (search_buffer == eocd_signature) {
			char buffer[18];
			is.read(buffer, 18);
			if (is.gcount() == 18) {
				uint16_t comment_size = *(reinterpret_cast<uint16_t*>(buffer + 16));
				if (comment_size > 0)
					copy_n(istream_iterator<char>(is), comment_size, back_inserter(comment));
				if (comment.size() == comment_size && is.peek() == EOF) {
					offset = *(reinterpret_cast<size_t*>(buffer + 12));
					eocd_found = true;
				}
			}
		}
		++back;
	}

	if (!eocd_found)
		error(".zip file does not contain the eocd record signature. Please verify that the is not corrupted.");

	if (!comment.empty())
		cout << "The comment is: " << comment << "\n";
	else
		cout << "There is no comment\n";
	return offset;
}

struct central_directory_file_header {
	uint16_t compression, disk_number_start;
	size_t compressed_size, uncompressed_size, offset;
	string filename, comment;
};

inline vector<central_directory_file_header> read_central_directory_file_headers(istream& is) {
	const array<char, 4> file_header_signature = { 0x50, 0x4b, 0x01, 0x02 };

	vector<central_directory_file_header> elems;
	array<char, 4> search_buffer;
	char buffer[42];
	while (true) {
		copy_n(istream_iterator<char>(is), 4, begin(search_buffer));
		if (search_buffer != file_header_signature)
			break;
		is.read(buffer, 42);
		central_directory_file_header h;
		h.compression = *(reinterpret_cast<uint16_t*>(buffer + 6));
		h.disk_number_start = *(reinterpret_cast<uint16_t*>(buffer + 30));
		h.compressed_size = *(reinterpret_cast<size_t*>(buffer + 16));
		h.uncompressed_size = *(reinterpret_cast<size_t*>(buffer + 20));
		h.offset = *(reinterpret_cast<size_t*>(buffer + 38));
		uint16_t name_len = *(reinterpret_cast<uint16_t*>(buffer + 24));
		uint16_t ex_len = *(reinterpret_cast<uint16_t*>(buffer + 26));
		uint16_t comment_len = *(reinterpret_cast<uint16_t*>(buffer + 28));
		copy_n(istream_iterator<char>(is), name_len, back_inserter(h.filename));
		is.seekg(ex_len, ios::cur);
		if (comment_len > 0)
			copy_n(istream_iterator<char>(is), comment_len, back_inserter(h.comment));
		elems.push_back(h);
	}

	if (elems.empty())
		error("Zip do not contains any file.");
	return elems;
}

inline void copy_uncompressed_file(istream& is, const vector<central_directory_file_header>& elems) {
	const array<char, 4> local_file_header_signature = { 0x50, 0x4B, 0x03, 0x04 };
	
	array<char, 4> search_buffer;
	char buffer[26];
	cout << "Write output files: \n";
	for (const auto& e : elems) {
		is.seekg(e.offset, ios::beg);
		copy_n(istream_iterator<char>(is), 4, begin(search_buffer));
		if (search_buffer != local_file_header_signature)
			error("No file starts here.");
		is.read(buffer, 26);
		size_t compressed_size = *(reinterpret_cast<size_t*>(buffer + 14));
		size_t uncompressed_size = *(reinterpret_cast<size_t*>(buffer + 18));
		uint16_t compression = *(reinterpret_cast<uint16_t*>(buffer + 4));
		uint16_t filename_len = *(reinterpret_cast<uint16_t*>(buffer + 22));
		uint16_t ex_len = *(reinterpret_cast<uint16_t*>(buffer + 24));
		string filename;
		copy_n(istream_iterator<char>(is), filename_len, back_inserter(filename));
		is.seekg(ex_len, ios::cur);
		uint16_t flags = *(reinterpret_cast<uint16_t*>(buffer + 2));
		if ((flags & 0x04) != 0) {
			char l_buf[12];
			is.read(l_buf, 12);
			compressed_size = *(reinterpret_cast<size_t*>(l_buf + 4));
			uncompressed_size = *(reinterpret_cast<size_t*>(l_buf + 8));
		}
		if (e.compression == 0) {
			if (e.filename != filename || e.compressed_size != compressed_size ||
				e.compression != compression || e.uncompressed_size != compressed_size)
				error("Local file header and central directory file header do not match.");
			ofstream os(e.filename, ios::binary);
			if (!os)
				error("Cannot open output file " + e.filename);
			copy_n(istream_iterator<char>(is), e.uncompressed_size, ostream_iterator<char>(os));
			cout << "Written file " << e.filename << "\n";
		}
	}
	cout << "\n";
}

void zip_struct(const string& input_filename) {
	if (!check_extension(input_filename, ".zip"))
		error("Input file must be a .zip file.");
	ifstream is(input_filename, ios::binary);
	if (!is)
		error("Cannot open the input file.");
	is.unsetf(ios::skipws);
	auto cd_offset = search_eocd(is);
	is.clear();
	is.seekg(cd_offset, ios::beg);
	auto elems = read_central_directory_file_headers(is);
	cout << "\nList of files in the .zip archive: \n";
	for (const auto& e : elems)
		cout << e.filename << "\n";
	cout << "\n";
	copy_uncompressed_file(is, elems);
}

int main(int argc, char **argv) {
	if (argc != 2)
		syntax();

	string input(argv[1]);
	zip_struct(input);
	cout << "Done!!\n";

	return EXIT_SUCCESS;
}