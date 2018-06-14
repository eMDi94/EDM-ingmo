#include "core.hpp"
#include "ppm.hpp"
#include "patient.hpp"
#include "dicom.hpp"
#include <string>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <array>

using namespace std;
using namespace core;
using namespace ppm;
using namespace patient_ns;
using namespace dicom;


void syntax() {
	cout << "Usage: txt2dcm <input_filename>.txt <output_filename>.dcm\n";
	exit(EXIT_FAILURE);
}

void error(string&& error_message) {
	cout << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

inline bool check_extension(const string& filename, string&& extension) {
	return filename.substr(filename.size() - extension.size()) == extension;
}

inline bool check_patient(const patient& p) {
	if (p.empty())
		return false;
	array<string, 6> compulsory_fields = { "File", "Surname", "Name", "Sex", "ID", "Implementation" };
	for (const auto& str : compulsory_fields)
		if (p.find(str) == end(p))
			return false;
	return true;
}

inline void write_group2(const dicom_writer& writer, const patient& p) {
	const uint16_t group = 0x0002;
	string vr;
	string str;
	writer.write_value(group, 0x0000, vr = "UL", 0);
	array<uint8_t, 2> bytes = { 0x00, 0x01 };
	writer.write_byte_stream(group, 0x0001, bytes.data(), 2);
	writer.write_string(group, 0x0002, vr = "UI", str = "1.2.840.10008.5.1.4.1.1.77.1.4");
	writer.write_string(group, 0x0003, vr, str = "1.2.392.200036.9125.0.19950720112207");
	writer.write_string(group, 0x0010, vr, str = "1.2.840.10008.1.2.1");
	writer.write_string(group, 0x0012, vr, p.at("Implementation"));
	writer.write_group2_size();
	if (!writer)
		error("Something happened during writing the group 2.");
}

inline void write_group8(const dicom_writer& writer) {
	const uint16_t group = 0x0008;
	string vr, str;
	writer.write_string(group, 0x0008, vr = "CS", str = "ORIGINAL\\PRIMARY");
	writer.write_string(group, 0x0016, vr = "UI", str = "1.2.840.10008.5.1.4.1.1.77.1.4");
	writer.write_string(group, 0x0018, vr, str = "1.2.392.200036.9125.0.19950720112207");
	if (!writer)
		error("Something happened during writing the group 8.");
}

inline void write_group10(const dicom_writer& writer, const patient& p) {
	writer.write_patient_name(p.at("Name"), p.at("Surname"));
	const uint16_t group = 0x0010;
	string vr;
	writer.write_string(group, 0x0020, vr = "LO", p.at("ID"));
	writer.write_string(group, 0x0040, vr = "CS", p.at("Sex"));
	if (!writer)
		error("Something happened during writing the group 10.");
}

inline void write_group28(const dicom_writer& writer, size_t height, size_t width) {
	const uint16_t group = 0x0028;
	string vr, str;
	writer.write_value(group, 0x0002, vr = "US", 3);
	writer.write_string(group, 0x0004, vr = "CS", str = "RGB");
	writer.write_value(group, 0x0006, vr = "US", 0);
	writer.write_value(group, 0x0010, vr, height);
	writer.write_value(group, 0x0011, vr, width);
	writer.write_value(group, 0x0100, vr, 8);
	writer.write_value(group, 0x0101, vr, 8);
	writer.write_value(group, 0x0102, vr, 7);
	writer.write_value(group, 0x0103, vr, 0);
	writer.write_string(group, 0x2110, vr = "CS", str = "00");
	if (!writer)
		error("Something happened during writing the group 28.");
}

inline void write_image(const dicom_writer& writer, const mat<vec3b>& image) {
	writer.write_byte_stream(0x7FE0, 0x0010, image.data(), image.height()*image.width() * 3);
}

void txt2dcm(const string& input_filename, const string& output_filename) {
	if (!check_extension(input_filename, ".txt") || !check_extension(output_filename, ".dcm"))
		error("Input file must be a .txt file and output file must be a .dcm file.");

	ifstream is(input_filename);
	if (!is)
		error("Can not open input file.");

	const patient p = parse(is);
	if (!check_patient(p))
		error("Some compulsory fields are missing from the patient file.");

	ofstream os(output_filename, ios::binary);
	if (!os)
		error("Can not open output file.");

	const dicom_writer writer(os);
	write_group2(writer, p);
	write_group8(writer);
	write_group10(writer, p);

	ifstream im(p.at("File"), ios::binary);
	mat<vec3b> image;
	if (!load_ppm(im, image))
		error("Problems during reading the image.");

	write_group28(writer, image.height(), image.width());
	write_image(writer, image);
}


int main(const int argc, char **argv) {
	if (argc != 3)
		syntax();

	const string input_filename(argv[1]);
	const string output_filename(argv[2]);

	txt2dcm(input_filename, output_filename);
	cout << "Done!!!\n";

	return EXIT_SUCCESS;
}