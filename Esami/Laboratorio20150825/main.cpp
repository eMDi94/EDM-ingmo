#include <iostream>
#include <fstream>
#include "core.hpp"
#include "ppm.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <iterator>


using namespace std;
using namespace core;
using namespace ppm;


void syntax() {
	cout << "Usage: ppm2dcm <input_file>.ppm <output_file>.dcm\n";
	exit(EXIT_FAILURE);
}

void error(string&& error_message) {
	cout << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

bool check_extension(const string& filename, string&& extension) {
	return filename.substr(filename.size() - extension.size()) == extension;
}


inline void write_header(ostream& os) {
	fill_n(ostream_iterator<uint8_t>(os), 128, uint8_t(0));
	os << "DICM";
	if (!os)
		error("Problem during writing the header.");
}

inline void write_value(ostream& os, const int64_t val, const size_t count) {
	os.write(reinterpret_cast<const char*>(&val), count);
}

inline void write_str(ostream& os, uint16_t group, uint16_t element, string&& vr, string&& str, char append_value) {
	write_value(os, group | (element << 16), 4);
	os << vr;
	size_t size = str.size();
	bool add = false;
	if (size % 2 != 0) {
		size += 1;
		add = true;
	}
	write_value(os, size, 2);
	os << str;
	if (add)
		os << append_value;
}

inline void write_group2(ostream& os) {
	const int64_t group2_pos = os.tellp();

	write_value(os, (0x0002) | (0x0000 << 16), 4);
	os << "UL";
	write_value(os, 4, 2);
	write_value(os, 0, 4);

	write_value(os, 0x0002 | (0x0001 << 16), 4);
	os << "OB";
	write_value(os, 0x0000 | (0x00000002 << 16), 6);
	write_value(os, 0x0100, 2);

	write_str(os, 0x0002, 0x0002, "UI", "1.2.840.10008.5.1.4.1.1.77.1.4", 0);
	write_str(os, 0x0002, 0x0003, "UI", "1.2.392.200036.9125.0.19950720112207", 0);
	write_str(os, 0x002, 0x0010, "UI", "1.2.840.10008.1.2.1", 0);
	write_str(os, 0x0002, 0x0012, "UI", "1.2.392.200036.9125.0.1234567890", 0);

	int64_t size = os.tellp() - group2_pos;
	size -= 12;
	os.seekp(group2_pos + 8, ios::beg);
	write_value(os, size, 4);
	os.seekp(0, ios::end);

	if (!os)
		error("Problems during writing group 2.");
}

inline void write_group8(ostream& os) {
	write_str(os, 0x0008, 0x0008, "CS", "ORIGINAL\\PRIMARY", 32);
	write_str(os, 0x0008, 0x0016, "UI", "1.2.840.10008.5.1.4.1.1.77.1.4", 0);
	write_str(os, 0x0008, 0x0018, "UI", "1.2.392.200036.9125.0.19950720112207", 0);

	if (!os)
		error("Problems during writing the group 8.");
}

inline void write_us(ostream& os, uint16_t group, uint16_t element, uint16_t value) {
	write_value(os, group | (element << 16), 4);
	os << "US";
	write_value(os, 2, 2);
	write_value(os, value, 2);
}

inline void write_group28(ostream& os, const size_t height, const size_t width) {
	write_us(os, 0x0028, 0x0002, 3);
	write_str(os, 0x0028, 0x0004, "CS", "RGB", 32);
	write_us(os, 0x0028, 0x0006, 0);
	write_us(os, 0x0028, 0x0010, height);
	write_us(os, 0x0028, 0x0011, width);
	write_us(os, 0x0028, 0x0100, 8);
	write_us(os, 0x0028, 0x0101, 8);
	write_us(os, 0x0028, 0x0102, 7);
	write_us(os, 0x0028, 0x0103, 0);
	write_str(os, 0x0028, 0x2110, "CS", "00", 32);

	if (!os)
		error("Problems during writing the group 28.");
}

inline void write_image(ostream& os, const mat<vec3b>& image) {
	write_value(os, 0x7FE0 | (0x0010 << 16), 4);
	os << "OB";
	size_t count = image.width() * image.height() * 3;
	bool add = false;
	if (count % 2 != 0) {
		count += 1;
		add = true;
	}
	write_value(os, 0x0000 | (int64_t(count) << 16), 6);
	os.write(reinterpret_cast<const char*>(image.data()), image.height() * image.width() * 3);
	if (add)
		os << 0;

	if (!os)
		error("Error during writing the image.");
}

void ppm2dcm(const string& input_file, const string& output_file) {
	if (!check_extension(input_file, ".ppm") || !check_extension(output_file, ".dcm"))
		error("Input file must be a .ppm file and output file must be a .dcm file");
	ifstream is(input_file, ios::binary);
	if (!is)
		error("Can not open the input file.");

	mat<vec3b> image;
	if (!load_ppm(is, image))
		error("There was a problem during reading the input image.");

	ofstream os(output_file, ios::binary);
	if (!os)
		error("Can not open the output file.");
	write_header(os);
	write_group2(os);
	write_group8(os);
	write_group28(os, image.height(), image.width());
	write_image(os, image);
}


int main(const int argc, char **argv) {
	if (argc != 3)
		syntax();

	const string input_filename(argv[1]);
	const string output_filename(argv[2]);
	ppm2dcm(input_filename, output_filename);
	cout << "Done!!!\n";

	return EXIT_SUCCESS;
}