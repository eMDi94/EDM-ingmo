#include "core.h"
#include "ppm.h"
#include "ubjson.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <sstream>

using namespace std;
using namespace core;
using namespace ppm;
using namespace ubjson;

void syntax() {
	cerr << "Usage: ubj2ppm <input_filename>.ubj <output_filename>.ppm\n";
	exit(EXIT_FAILURE);
}

void error(string error_message) {
	cerr << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

inline bool check_extension(const string& filename, string extension) {
	return filename.substr(filename.size() - extension.size()) == extension;
}

inline void write_output(const mat<vec3b>& img, const string& output_filename) {
	ofstream os(output_filename, ios::binary);
	if (!os)
		error("Cannot open output file " + output_filename);
	if (!save_ppm(os, img))
		error("Cannot save output image on file " + output_filename);
}

inline mat<vec3b>& draw_canvas(const ubjson_ptr& root, mat<vec3b>& img) {
	const auto& v = root->obj();
	auto e = end(v);
	auto icanvas = find_if(begin(v), e, [](const auto& p) -> bool {
		return p.first == "canvas";
	});
	if (icanvas == e)
		error("Root object does not contains the canvas object.");
	if (icanvas->second->type() != vtype::object)
		error("Canvas is not an object.");
	const auto& canvas = icanvas->second->obj();
	e = end(canvas);
	size_t width = find_if(begin(canvas), e, [](const auto& p) -> bool {
		return p.first == "width";
	})->second->int_value();
	size_t height = find_if(begin(canvas), e, [](const auto& p) -> bool {
		return p.first == "height";
	})->second->int_value();
	const auto& background = find_if(begin(canvas), e, [](const auto& p) -> bool {
		return p.first == "background";
	})->second->ubj_array();
	img.resize(height, width);
	vec3b b;
	for (size_t i = 0; i < 3; ++i)
		b[i] = background[i]->uint8();
	fill(begin(img), end(img), b);
	return img;
}

inline mat<vec3b>& draw_images(const ubjson_ptr& root, mat<vec3b>& img) {
	const auto& v = root->obj();
	auto e = end(v);
	auto ielements = find_if(begin(v), e, [](const auto& p) -> bool {
		return p.first == "elements";
	});
	if (ielements == e)
		return img;
	if (ielements->second->type() != vtype::object)
		error("Elements must be an object.");
	size_t img_count = 0;
	for (const auto& e : ielements->second->obj()) {
		if (e.second->type() == vtype::object) {
			cout << e.first << ": ";
			for (const auto& attr : e.second->obj())
				cout << attr.first << ",";
			cout << "\n";
			if (e.first == "image") {
				const auto& eo = e.second->obj();
				size_t x = find_if(begin(eo), end(eo), [](const auto& p) -> bool {
					return p.first == "x";
				})->second->int_value();
				size_t y = find_if(begin(eo), end(eo), [](const auto& p) -> bool {
					return p.first == "y";
				})->second->int_value();
				size_t height = find_if(begin(eo), end(eo), [](const auto& p) -> bool {
					return p.first == "height";
				})->second->int_value();
				size_t width = find_if(begin(eo), end(eo), [](const auto& p) -> bool {
					return p.first == "width";
				})->second->int_value();
				const auto& data = find_if(begin(eo), end(eo), [](const auto& p) -> bool {
					return p.first == "data";
				})->second->ubj_array();
				mat<vec3b> local_img(height, width);
				size_t array_pos = 0;
				for (size_t r = 0; r < height; ++r) {
					for (size_t c = 0; c < width; ++c) {
						vec3b color;
						for (size_t k = 0; k < 3; ++k)
							color[k] = data[array_pos++]->uint8();
						local_img(r, c) = move(color);
					}
				}
				stringstream ss;
				ss << "image" << ++img_count << ".ppm";
				write_output(local_img, ss.str());
				for (size_t r = 0; r < height; ++r) {
					for (size_t c = 0; c < width; ++c) {
						img(y + r, x + c) = local_img(r, c);
					}
				}
			}
		}
	}

	return img;
}

void ubj2ppm(const string& input_filename, const string& output_filename) {
	if (!check_extension(input_filename, ".ubj"))
		error("Input file must be a .ubj file.");
	if (!check_extension(output_filename, ".ppm"))
		error("Output file must be a .ppm file.");

	ifstream is(input_filename, ios::binary);
	if (!is)
		error("Cannot open input file.");
	is.unsetf(ios::skipws);
	ubjson_ptr root = parse(is);
	mat<vec3b> img;
	draw_canvas(root, img);
	write_output(img, string("background.ppm"));
	draw_images(root, img);
	write_output(img, output_filename);
}

int main(int argc, char **argv) {
	if (argc != 3)
		syntax();

	string input(argv[1]);
	string output(argv[2]);
	ubj2ppm(input, output);
	cout << "\nDone!!\n";

	return EXIT_SUCCESS;
}