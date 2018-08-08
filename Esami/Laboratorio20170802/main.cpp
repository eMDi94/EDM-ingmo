#include "core.h"
#include "ppm.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <cstdlib>
#include <unordered_map>
#include <iterator>
#include <algorithm>
#include <tuple>

using namespace std;
using namespace core;
using namespace ppm;

void syntax() {
	cerr << "Usage: gif2ppm <input_filename>.gif <output_filename>.ppm\n";
	exit(EXIT_FAILURE);
}

void error(string error_message) {
	cerr << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

inline bool check_extension(const string& filename, string extension) {
	return filename.substr(filename.size() - extension.size()) == extension;
}

inline bool is_gif(istream& is) {
	string magic = "GIF87a";
	copy_n(istream_iterator<char>(is), 6, begin(magic));
	return magic == "GIF87a";
}

class gif87a_stream_reader {
public:
	explicit gif87a_stream_reader(istream& is) : is_(is), v_(), buffer_(0), count_(0) {}

	uint16_t get(size_t n) {
		uint16_t ret = 0;
		for (size_t i = 0; i < n; ++i)
			ret = ret | (get_bit() << i);
		return ret;
	}

private:
	istream& is_;
	vector<uint8_t> v_;
	uint8_t buffer_ = 0;
	uint8_t count_ = 0;

	uint16_t get_bit() {
		if (count_ == 0) {
			if (v_.empty()) {
				uint8_t block_byte_count;
				is_.read(reinterpret_cast<char*>(&block_byte_count), 1);
				v_.resize(block_byte_count);
				copy_n(istream_iterator<uint8_t>(is_), block_byte_count, begin(v_));
				reverse(begin(v_), end(v_));
			}
			buffer_ = v_.back();
			v_.pop_back();
			count_ = 8;
		}

		uint16_t bit = buffer_ & 0x01;
		buffer_ = buffer_ >> 1;
		--count_;
		return bit;
	}
};

inline void read_screen_descriptor(istream& is, mat<vec3b>& img, uint8_t& SD_flags, uint8_t& background_index) {
	char buffer[7];
	is.read(buffer, 7);
	uint16_t width = *(reinterpret_cast<uint16_t*>(buffer));
	uint16_t height = *(reinterpret_cast<uint16_t*>(buffer + 2));
	img.resize(height, width);
	SD_flags = *(reinterpret_cast<uint8_t*>(buffer + 4));
	background_index = *(reinterpret_cast<uint8_t*>(buffer + 5));
	if (buffer[6] != 0)
		error("Screen descriptor end with a non zero value.");
}

inline vector<vec3b> read_color_table(istream& is, size_t count) {
	vector<vec3b> colors(count);
	for (size_t i = 0; i < count; ++i) {
		vec3b color;
		is.read(reinterpret_cast<char*>(color.data()), 3);
		colors[i] = move(color);
	}
	return colors;
}

inline size_t bit_count(size_t dict_size) {
	size_t c;
	for (c = 0; dict_size != 0; dict_size = dict_size >> 1, ++c);
	return c;
}

inline vector<vec3b> read_raster_data(istream& is, const vector<vec3b>& color_table) {
	uint8_t code_size;
	is.read(reinterpret_cast<char*>(&code_size), 1);

	
	using lzw_pair = pair<uint16_t, vec3b>;
	uint16_t clear_code = 1 << code_size; // This is equal of doing pow(2, code_size)
	vector<lzw_pair> dictionary;
	for (uint16_t i = 0; i < clear_code; ++i)
		dictionary.emplace_back(4097ui16, color_table[i]);
	dictionary.emplace_back(); // Clear code
	dictionary.emplace_back(); // End Of Information
	uint16_t end_of_information = clear_code + 1;
	uint16_t last_decoded_code = 0;
	bool first = true;
	gif87a_stream_reader br(is);
	
	vector<vec3b> decoded;
	while (true) {
		size_t n_bit = bit_count(dictionary.size());
		n_bit = n_bit < 13 ? n_bit : 12;
		uint16_t code = br.get(n_bit);
		if (dictionary.size() == 4096 && code != clear_code)
			error("The stream is corrupted because the standard does not allow code greater than 12 bits.");
		if (code == end_of_information)
			break;
		if (code == clear_code) {
			dictionary.erase(begin(dictionary) + clear_code + 2, end(dictionary));
			first = true;
			continue;
		}
		if (!first) {
			uint16_t tmp = code != dictionary.size() ? code : last_decoded_code;
			vector<uint16_t> indexes;
			while (tmp != 4097) {
				indexes.push_back(tmp);
				tmp = dictionary[tmp].first;
			}

			for (auto r_it = rbegin(indexes); r_it != rend(indexes); ++r_it)
				decoded.push_back(dictionary[*r_it].second);

			vec3b& f_el = dictionary[indexes.back()].second;
			if (code == dictionary.size())
				decoded.push_back(f_el);

			dictionary.emplace_back(last_decoded_code, f_el);
		}
		else {
			decoded.push_back(dictionary[code].second);
			first = false;
		}
		last_decoded_code = code;
	}

	return decoded;
}

inline void read_image_data(istream& is, const vector<vec3b>& global_color_map, uint8_t bpp, mat<vec3b>& img) {
	vector<vec3b> local_colors;
	uint8_t local_bpp;
	while (is.peek() == ',') {
		char buffer[10];
		is.read(buffer, 10);
		uint16_t i_left = *(reinterpret_cast<uint16_t*>(buffer + 1));
		uint16_t i_top = *(reinterpret_cast<uint16_t*>(buffer + 3));
		uint16_t width = *(reinterpret_cast<uint16_t*>(buffer + 5));
		uint16_t height = *(reinterpret_cast<uint16_t*>(buffer + 7));
		uint8_t ID_flags = *(reinterpret_cast<uint8_t*>(buffer + 9));
		bool use_global_colors = true;
		if ((ID_flags & 0x80) != 0) {
			use_global_colors = false;
			local_bpp = (ID_flags & 0x07) + 1;
			local_colors = read_color_table(is, 1 << local_bpp);
		}
		vector<vec3b> decoded;
		if (use_global_colors)
			decoded = read_raster_data(is, global_color_map);
		else
			decoded = read_raster_data(is, local_colors);

		auto d_it = begin(decoded);
		for (uint16_t r = i_top; r < i_top + height && d_it != end(decoded); ++r)
			for (uint16_t c = i_left; c < i_left + width && d_it != end(decoded); ++c, ++d_it)
				img(r, c) = move(*d_it);
	}
}

void gif2ppm(const string& input_filename, const string& output_filename) {
	if (!check_extension(input_filename, ".gif"))
		error("Input file must be a .gif file.");
	if (!check_extension(output_filename, ".ppm"))
		error("Output file must be a .ppm file.");
	ifstream is(input_filename, ios::binary);
	if (!is)
		error("Cannot open input file.");
	is.unsetf(ios::skipws);

	if (!is_gif(is))
		error("Input file is not a GIF87a file.");
	mat<vec3b> img;
	uint8_t SD_flags, background_index;
	read_screen_descriptor(is, img, SD_flags, background_index);
	vector<vec3b> global_colors;
	uint8_t bits_per_pixel = (SD_flags & 0x07) + 1;
	if ((SD_flags & 0x80) != 0) {
		global_colors = read_color_table(is, 1 << bits_per_pixel);
		fill(begin(img), end(img), global_colors[background_index]);
	}

	read_image_data(is, global_colors, bits_per_pixel, img);

	ofstream os(output_filename, ios::binary);
	if (!os)
		error("Cannot open output file.");
	if (!save_ppm(os, img))
		error("Cannot save output image.");
}

int main(int argc, char **argv) {
	if (argc != 3)
		syntax();

	string input(argv[1]);
	string output(argv[2]);
	gif2ppm(input, output);
	cout << "Done!!\n";

	return EXIT_SUCCESS;
}