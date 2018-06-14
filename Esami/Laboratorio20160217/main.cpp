#include "image.h"
#include "ppm.h"
#include "pgm.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <map>
#include <array>
#include <sstream>
#include <iomanip>

using namespace std;
using namespace image;
using namespace ppm;
using namespace pgm;

void syntax() {
	cerr << "Syntax: y4mextract <inputfile>.y4m\n";
	exit(EXIT_FAILURE);
}

void error(string&& error_message) {
	cerr << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

inline bool check_extension(const string& filename, string&& extension) {
	return filename.substr(filename.size() - extension.size()) == extension;
}

inline pair<string, string> read_tag(char tag, istream& is) {
	string value;
	is >> value;

	switch(tag) {
	case 'W':
		return { "width", move(value) };
	case 'H':
		return { "height", move(value) };
	case 'C':
		return { "chroma_subsampling", move(value) };
	case 'I':
		return  { "interleaving", move(value) };
	case 'F':
		return { "frame_rate", move(value) };
	case 'A':
		return { "aspect_ratio", move(value) };
	case 'X':
		return { "X", move(value) };
	default:
		string error_message("Tag ");
		error_message += tag;
		error_message += " not defined by the standard.";
		error(move(error_message));
		return {};
	}
}

map<string, string> read_stream_header(istream& is) {
	string magic;
	copy_n(istream_iterator<char>(is), 9, back_inserter(magic));
	if (magic != "YUV4MPEG2")
		error("The file do not start with a YUV4MPEG2 string");

	map<string, string> tagged_fields;
	while (is.peek() != '\n') {
		if (is.peek() != ' ')
			error("TAGGED FIELD must start with a whitespace.");
		char tag;
		is >> ws >> tag;
		tagged_fields.insert(read_tag(tag, is));
	}

	is.get();
	if (tagged_fields.find("width") == end(tagged_fields))
		error("STREAM HEADER must contain information about the width of the frames.");
	if (tagged_fields.find("height") == end(tagged_fields))
		error("STREAM HEADER must contain information about the height of the frames.");
	if (tagged_fields.find("chroma_subsampling") == end(tagged_fields))
		tagged_fields.insert(make_pair("chroma_subsampling", "420jpeg"));

	return tagged_fields;
}

inline void read_frame_header(istream& is) {
	string magic;
	copy_n(istream_iterator<char>(is), 5, back_inserter(magic));

	if (magic != "FRAME")
		error("Header of the frame must start with FRAME tag.");

	while (is.peek() != '\n') {
		char tag;
		string value;
		is >> ws >> tag >> value;
	}
	is.get();
}

uint8_t saturate_y(const uint8_t y) {
	return y <= 16 ? 16 : y >= 235 ? 235 : y;
}

uint8_t saturate_chroma(const uint8_t chroma) {
	return chroma <= 16 ? 16 : chroma >= 240 ? 240 : chroma;
}

array<mat<uint8_t>, 3> read_frame(istream& is, size_t height, size_t width) {
	read_frame_header(is);
	size_t mid_height = size_t(ceil(height / 2.));
	size_t mid_width = size_t(ceil(width / 2.));

	// Read the y channel
	mat<uint8_t> y(height, width);
	is.read(reinterpret_cast<char*>(y.data()), height*width);

	transform(begin(y), end(y), begin(y), saturate_y);


	// Read the cb channel
	mat<uint8_t> cb(mid_height, mid_width);
	is.read(reinterpret_cast<char*>(cb.data()), mid_height*mid_width);

	// Read the cr channel
	mat<uint8_t> cr(mid_height, mid_width);
	is.read(reinterpret_cast<char*>(cr.data()), mid_height*mid_width);

	return { y, cb, cr };
}

inline void write_y_channel(const mat<uint8_t>& y_channel, const string& filename) {
	ofstream os(filename, ios::binary);
	if (!os)
		error("Cannot open the output file for the y channel.");

	if (!save_pgm(os, y_channel))
		error("Problems during saving the y channel.");
}

inline string compose_index(size_t index) {
	stringstream ss;
	ss << setw(4) << setfill('0') << index;
	return ss.str();
}

mat<uint8_t> scale_up(const mat<uint8_t>& channel, size_t height, size_t width) {
	mat<uint8_t> scaled_up(height, width);

	for (size_t r = 0; r < height; ++r)
		for (size_t c = 0; c < width; ++c)
			scaled_up(r, c) = channel(r / 2, c / 2);

	return scaled_up;
}

mat<uint8_t> bilinear_scale_up(const mat<uint8_t>& channel, size_t height, size_t width) {
	mat<uint8_t> out(height, width);

	for (size_t r = 0; r < height; ++r) {
		double dr = (r + 0.5) / 2 - 0.5;
		size_t rl = size_t(max(0., floor(dr)));
		size_t rh = min(rl + 1, channel.height() - 1);
		double wrh = dr - rl;
		double wrl = 1 - wrh;
		for (size_t c = 0; c < width; ++c) {
			double dc = (c + 0.5) / 2 - 0.5;
			size_t cl = size_t(max(0., floor(dc)));
			size_t ch = min(cl + 1, channel.width() - 1);
			double wch = dc - cl;
			double wcl = 1 - wch;

			uint8_t ll = channel(rl, cl);
			uint8_t hl = channel(rh, cl);
			uint8_t lh = channel(rl, ch);
			uint8_t hh = channel(rh, ch);

			out(r, c) = uint8_t(round(ll*wrl*wcl + lh * wrl*wch + hl * wrh*wcl + hh * wrh*wch));
		}
	}

	return out;
}

mat<vec3b> compose(const array<mat<uint8_t>, 3>& channels) {
	size_t height = channels[0].height();
	size_t width = channels[0].width();
	mat<vec3b> out(height, width);

	for (size_t r = 0; r < height; ++r)
		for (size_t c = 0; c < width; ++c)
			for (size_t i = 0; i < 3; ++i)
				out(r, c)[i] = channels[i](r, c);

	return out;
}

vec3b YCbCr2RGB(const vec3b& YCbCr) {
	double y = YCbCr[0] - 16.;
	double cb = YCbCr[1] - 128.;
	double cr = YCbCr[2] - 128.;

	double r = y * 1.164 + 1.596 * cr;
	double g = y * 1.164 - cb * 0.392 - cr * 0.813;
	double b = y * 1.164 + cb * 2.017;

	uint8_t R = r < 0. ? uint8_t(0) : r > 255. ? uint8_t(255) : uint8_t(r);
	uint8_t G = g < 0. ? uint8_t(0) : g > 255. ? uint8_t(255) : uint8_t(g);
	uint8_t B = b < 0. ? uint8_t(0) : b > 255. ? uint8_t(255) : uint8_t(b);

	return { R, G, B };
}

inline void write_ppm(const mat<vec3b>& img, const string& filename) {
	ofstream os(filename, ios::binary);
	if (!os)
		error("Error on opening an output file.");

	if (!save_ppm(os, img))
		error("Error on saving an output file.");
}

void y4mextract(const string& input_file) {
	if (!check_extension(input_file, ".y4m"))
		error("Input file must be a .y4m file.");
	ifstream is(input_file, ios::binary);
	if (!is)
		error("Cannot open input file.");

	const auto header_fields = read_stream_header(is);
	if (header_fields.at("chroma_subsampling") != "420jpeg")
		error("I'm sorry. This program supports only 420jpeg.");

	const size_t height = size_t(stoi(header_fields.at("height")));
	const size_t width = size_t(stoi(header_fields.at("width")));

	size_t index = 0;
	while (is.peek() == 'F') {
		array<mat<uint8_t>, 3> frame = read_frame(is, height, width);

		const string index_string = compose_index(++index);
		const string y_channel = "extract/Y" + index_string + ".pgm";
		write_y_channel(frame[0], y_channel);

		array<mat<uint8_t>, 3> simple_scale;
		array<mat<uint8_t>, 3> bilinear_scale_up;
		simple_scale[0] = bilinear_scale_up[0] = frame[0];
		for (size_t i = 1; i < 3; ++i) {
			simple_scale[i] = scale_up(frame[i], height, width);
			transform(begin(simple_scale[i]), end(simple_scale[i]), begin(simple_scale[i]), saturate_chroma);
			bilinear_scale_up[i] = ::bilinear_scale_up(frame[i], height, width);
			transform(begin(bilinear_scale_up[i]), end(bilinear_scale_up[i]), begin(bilinear_scale_up[i]), saturate_chroma);
		}

		array<mat<vec3b>, 2> rebuilt_frames;
		rebuilt_frames[0] = compose(simple_scale);
		rebuilt_frames[1] = compose(bilinear_scale_up);
		

		for (auto& f : rebuilt_frames)
			transform(begin(f), end(f), begin(f), YCbCr2RGB);

		string fr = "extract/frame" + index_string + ".ppm";
		write_ppm(rebuilt_frames[0], fr);
		string inter = "extract/inter" + index_string + ".ppm";
		write_ppm(rebuilt_frames[1], inter);
	}
}


int main(const int argc, char **argv) {
	if (argc != 2)
		syntax();

	const string input(argv[1]);
	y4mextract(input);
	cout << "Done!!!\n";

	return EXIT_SUCCESS;
}