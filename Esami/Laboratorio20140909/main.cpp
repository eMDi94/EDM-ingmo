#include "bitmap.h"
#include "core.h"
#include "ppm.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>

using namespace std;
using namespace core;
using namespace ppm;
using namespace bmp;

void syntax() {
	cerr << "Usage: <input_file>.bmp <output_file>.ppm\n";
	exit(EXIT_FAILURE);
}

void error(string error_message) {
	cerr << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

inline bool check_extension(const string& filename, string extension) {
	return filename.substr(filename.size() - extension.size()) == extension;
}

inline void convert(const string& input_filename, const string& output_filename) {
	if (!check_extension(input_filename, ".bmp"))
		error("Input file must be a .bmp file.");
	if (!check_extension(output_filename, ".ppm"))
		error("Output file must be a .ppm file.");

	ifstream is(input_filename, ios::binary);
	if (!is)
		error("Cannot open the input file.");
	mat<vec3b> img;
	if (!load_bmp(is, img))
		error("Cannot read the bmp image from input file.");
	
	ofstream os(output_filename, ios::binary);
	if (!os)
		error("Cannot open the output file.");
	if (!save_ppm(os, img))
		error("Cannot save the image on output file.");
}


int main(int argc, char **argv) {
	if (argc != 3)
		syntax();

	string input(argv[1]);
	string output(argv[2]);
	convert(input, output);
	cout << "Done!!\n";

	return EXIT_SUCCESS;
}