#include <iostream>
#include <sstream>
#include <cstdint>
#include <fstream>
#include "core.h"
#include "ppm.h"
#include "vector_graphics.h"
#include <cstdlib>
#include <string>
#include <array>
#include <algorithm>
#include <iomanip>

using namespace std;
using namespace core;
using namespace vector_graphics;
using namespace ppm;

//////////////////////////////
/*Error handling*/
//////////////////////////////

void syntax() {
	cout << "Usage: image_extract <input_file>.txt <output_file_prefix>\n";
	exit(EXIT_FAILURE);
}

void error(string&& error_message) {
	cout << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

//////////////////////////////////
/*Check file extension*/
//////////////////////////////////

inline bool check_extension(const string& filename, string&& extension) {
	return filename.substr(filename.size() - extension.size()) == extension;
}


/***********************************************************
 * Check if image object contains all the required field****
 ***********************************************************/
inline bool check_image_object(const element& image_element) {
	if (!image_element.contains("data"))
		return false;
	const element& data_element = image_element["data"];
	const array<string, 3> required_data_fields = { "height", "width", "pixel" };
	for (const auto& field : required_data_fields)
		if (!data_element.contains(field) && data_element[field].type() != type::value)
			return false;
	return true;
}

char char_to_b64(char c) {
	if (c >= 65 && c <= 90)
		return c - 65;
	if (c >= 97 && c <= 122)
		return c - 71;
	if (c >= 48 && c <= 57)
		return c + 4;
	if (c == 43)
		return 62;
	if (c == 47)
		return 63;
	if (c == '=')
		return c;
	error("Char in econding base 64 not recognized.");
	return 0;
}

vec3b pixel_decode(array<char, 4>& b64_chars) {
	// simplified version
	transform(begin(b64_chars), end(b64_chars), begin(b64_chars), char_to_b64);

	vec3b pixel;
	pixel[0] = (b64_chars[0] << 2 | ((b64_chars[1] & 0x30) >> 4));
	pixel[1] = (((b64_chars[1] & 0x0F) << 4) | (((b64_chars[2] & 0x3C) >> 2)));
	pixel[2] = (((b64_chars[2] & 0x03) << 6) | ((b64_chars[3] & 0x3F)));

	return pixel;
}

mat<vec3b> decode(const size_t height, const size_t width, const value& pixels) {
	mat<vec3b> image(height, width);

	auto str_it = begin(pixels);
	for (size_t r = 0; r < height; ++r) {
		for (size_t c = 0; c < width; ++c) {
			array<char, 4> b64_chars{};
			copy_n(str_it, 4, begin(b64_chars));
			str_it += 4;
			image(r, c) = pixel_decode(b64_chars);
		}
	}

	return image;
}

void write_image(const mat<vec3b>& image, const string& output_prefix, size_t& index) {
	stringstream ss;
	ss << setw(4) << setfill('0') << int32_t(index);

	ofstream os(output_prefix + ss.str() + ".ppm", ios::binary);
	if (!os)
		error("Error on opening the output image.");

	if (!save_ppm(os, image))
		error("Error on saving an image.");
}


void visit(const element& el, const string& output_prefix, size_t& index) {
	if (el.is_hidden())
		return;
	if (el.element_name() == "image") {
		if (!check_image_object(el))
			error("Image object not formatted correctly.");
		const element& data = el["data"];
		const size_t height = size_t(stoi(data["height"].value()));
		const size_t width = size_t(stoi(data["width"].value()));
		value pixels = data["pixel"].value();
		size_t i = string::npos;
		while ((i = pixels.find('\n')) != string::npos)
			pixels.replace(i, 1, "");
		while ((i = pixels.find(' ')) != string::npos)
			pixels.replace(i, 1, "");
		const mat<vec3b> image = decode(height, width, pixels);
		write_image(image, output_prefix, index);
	}
	if (el.type() == type::object)
		for (const auto& e : el.object())
			if (e.type() == type::object)
				visit(e, output_prefix, index);
}


void extract(const string& input_filename, const string& output_prefix) {
	if (!check_extension(input_filename, ".txt"))
		error("Input file must be a .txt file.");

	ifstream is(input_filename);
	if (!is)
		error("Can not open input file.");

	const element root_element = parse(is);

	size_t index = 1;
	visit(root_element, output_prefix, index);
}



int main(const int argc, char **argv) {
	if (argc != 3)
		syntax();

	const string input_filename(argv[1]);
	const string output_prefix(argv[2]);

	extract(input_filename, output_prefix);
	cout << "Done!!!\n";

	return EXIT_SUCCESS;
}