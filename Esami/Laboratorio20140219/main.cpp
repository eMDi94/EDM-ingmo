#include "core.h"
#include "ppm.h"
#include "json.h"
#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>


using namespace std;
using namespace core;
using namespace ppm;
using namespace Json;


/*
 * Error handling
 */

void syntax() {
	cout << "Usage: json2ppm <input_filename>.txt <output_filename>.ppm\n";
	exit(EXIT_FAILURE);
}

void error(string&& error_message) {
	cout << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

bool check_extension(const string& filename, string&& extension) {
	return filename.substr(filename.size() - extension.size()) == extension;
}

/*
 * End of error handling
 */


/*
 * Conversion from json to ppm
 */

inline vec3b hex2vec3b(const string& color_string) {
	stringstream ss(color_string);
	int32_t value;
	ss >> hex >> value;
	const vec3b background{ uint8_t((value & 0xFF0000) >> 16), uint8_t((value & 0xFF00) >> 8), uint8_t(value & 0xFF) };
	return background;
}

void check_parsed_json(const json& item) {
	if (item.type() != json_type::object)
		error("Root element is not an object.");

	const object& obj = item.object_items();
	const std::array<string, 4> values = { "height", "width", "background", "elements" };
	for (const auto& value : values)
		if (obj.find(value) == end(obj))
			error("Object does not contains the requested values.");
}

mat<vec3b> fill_background(const json& item) {
	const size_t height = size_t(item["height"].int_value());
	const size_t width = size_t(item["width"].int_value());

	mat<vec3b> image(height, width);
	const string& background_string = item["background"].string_value();

	const vec3b background = hex2vec3b(background_string);

	fill(begin(image), end(image), background);

	return image;
}

void add_elements(mat<vec3b>& image, const Json::array& elements) {
	for (const auto& element : elements) {
		if (element.type() != json_type::object)
			error("One of the value in the elements array is not an object.");
		
		const size_t x = size_t(element["x"].int_value());
		const size_t y = size_t(element["y"].int_value());
		const size_t width = size_t(element["width"].int_value());
		const size_t height = size_t(element["height"].int_value());

		const string& color_string = element["color"].string_value();
		const vec3b color = hex2vec3b(color_string);


		for (size_t r = y; r < y + height; ++r)
			for (size_t c = x; c < x + width; ++c)
				image(r, c) = color;
	}
}

void json2ppm(const string& input_filename, const string& output_filename) {
	try {
		if (!check_extension(input_filename, ".json"))
			error("Input file is not a json file.");
		if (!check_extension(output_filename, ".ppm"))
			error("Output file must be a .ppm file.");
		
		ifstream is(input_filename);
		if (!is)
			error("Input file not found");
		
		const json item = parse(is);
		check_parsed_json(item);
		
		mat<vec3b> image = fill_background(item);

		add_elements(image, item["elements"].array_items());

		ofstream os(output_filename, ios::binary);
		if (!os)
			error("Can not open output file.");
		if (!save_ppm(os, image))
			error("Somethin went wrong during writing the output image.");
	}
	catch (logic_error& er) {
		error(er.what());
	}
}

/*
 * End of conversion
 */


int main(const int argc, char **argv) {
	if (argc != 3)
		syntax();

	const string input_filename(argv[1]);
	const string output_filename(argv[2]);
	json2ppm(input_filename, output_filename);
	cout << "Done!!!\n";

	return EXIT_SUCCESS;
}