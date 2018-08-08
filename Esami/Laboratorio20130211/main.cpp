#include "core.h"
#include "posimage.h"
#include "ppm.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iterator>
#include <algorithm>
#include <cstdint>
#include <cstdlib>

using namespace std;
using namespace core;
using namespace pim;
using namespace ppm;

void syntax() {
	cerr << "Usage: pim2ppm <input_filename>.pim <output_filename>.ppm\n";
	exit(EXIT_FAILURE);
}

void error(string error_message) {
	cerr << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

inline bool check_extension(const string& filename, string extension) {
	return filename.substr(filename.size() - extension.size()) == extension;
}

class vector_bit_reader {
public:
	explicit vector_bit_reader(const vector<uint8_t>& v) :
		it_(begin(v)), end_(end(v)), buffer_(0), count_(0) {}

	uint8_t get(size_t n) {
		uint8_t ret = 0;
		while (n-- > 0)
			ret = (ret << 1) | get_bit();
		return ret;
	}

	bool eos() const {
		return it_ == end_ ? count_ == 0 : false;
	}

private:
	vector<uint8_t>::const_iterator it_;
	vector<uint8_t>::const_iterator end_;
	uint8_t buffer_;
	uint8_t count_;

	uint8_t get_bit() {
		if (count_ == 0) {
			buffer_ = *it_;
			++it_;
			count_ = 8;
		}

		return ((buffer_ >> (--count_)) & 0x01);
	}
};

inline void check_header(const vector<pim_ptr>& ptrs) {
	for (const auto& p : ptrs) {
		if (p->id() == 0x0A45DFA3) {
			const auto& elements = p->elements();
			for (const auto& e : elements) {
				if (e->id() == 0x0282)
					cout << "DocType: " << e->str_value() << "\n";
				else
					if (e->id() == 0x0286)
						cout << "EBMLVersion: " << e->uint_value() << "\n";
			}
			return;
		}
	}
	error("pim file must contains EBML Header.");
}

inline pair<size_t, size_t> read_image_info(const vector<pim_ptr>& ptrs) {
	size_t width = 0, height = 0;
	for (const auto& p : ptrs) {
		if (p->id() == 0x20) {
			const auto& elements = p->elements();
			for (const auto& e : elements) {
				if (e->id() == 0x21)
					width = e->uint_value();
				else
					if (e->id() == 0x22)
						height = e->uint_value();
			}
			if (width == 0 || height == 0)
				error("Width or Height cannot be 0.");
			return make_pair(height, width);
		}
	}
	error("Something went wrong during parsing the image info.");
	return make_pair(size_t(0), size_t(0));
}

const vector<uint8_t>& obtain_img_data(const vector<pim_ptr>& ptrs) {
	for (const auto& p : ptrs) {
		if (p->id() == 0x30)
			return p->binary();
	}
	throw runtime_error("LOL");
}

void pim2ppm(const string& input_filename, const string& output_filename) {
	if (!check_extension(input_filename, ".pim"))
		error("Input file must be a .pim file.");
	if (!check_extension(output_filename, ".ppm"))
		error("Output file must be a .ppm file.");

	ifstream is(input_filename, ios::binary);
	if (!is)
		error("Cannot open input file.");
	is.unsetf(ios::skipws);
	vector<pim_ptr> ptrs = parse_multi(is);
	check_header(ptrs);
	auto img_info = read_image_info(ptrs);
	mat<vec3b> img(img_info.first, img_info.second);
	const auto& data = obtain_img_data(ptrs);

	vector_bit_reader br(data);

	for (size_t r = 0; r < img.height(); ++r) {
		for (size_t c = 0; c < img.width(); ++c) {
			if (br.get(1) == 0) {
				uint8_t red = br.get(8), green = br.get(8), blue = br.get(8);
				img(r, c) = { red, green, blue };
			}
			else {
				img(r, c) = { uint8_t(255), uint8_t(0), uint8_t(255) };
			}
		}
	}

	ofstream os(output_filename, ios::binary);
	if (!os)
		error("Cannot open output file.");
	if (!save_ppm(os, img))
		error("Cannot save output file.");
}

int main(int argc, char **argv) {
	if (argc != 3)
		syntax();

	string input(argv[1]);
	string output(argv[2]);
	pim2ppm(input, output);
	cout << "Done!!\n";

	return EXIT_SUCCESS;
}