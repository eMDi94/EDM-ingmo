#include "bit.h"
#include "frequencies.h"
#include "huffman.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <numeric>

#define MULTIPLIER 16777215U

using namespace std;
using namespace bit;
using namespace frequencies;
using namespace huffman;

void syntax() {
	cerr << "Usage: doublehuffenc <input_filename>.txt <output_filename>.bin\n";
	exit(EXIT_FAILURE);
}

void error(string error_message) {
	cerr << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

inline bool check_extension(const string& filename, string extension) {
	return filename.substr(filename.size() - extension.size()) == extension;
}

template<typename _Fn>
inline void read_file(istream& is, _Fn& f) {
	while (is.peek() != EOF) {
		double v;
		is >> v >> ws;
		if (v < 0. || v > 1.)
			error("Only values in interval [0, 1] are allowed.");
		if (is.peek() == ',')
			is.get();
		uint32_t value = uint32_t(floor(v * MULTIPLIER));
		f(value);
	}
}

void encode(const string& input_filename, const string& output_filename) {
	if (!check_extension(input_filename, ".txt"))
		error("Input file must be a .txt file.");
	if (!check_extension(output_filename, ".bin"))
		error("Output file must be .bin file.");
	ifstream is(input_filename);
	if (!is)
		error("Cannot open the input file.");
	frequencies_counter<uint32_t> f;
	read_file(is, f);
	is.clear();
	is.seekg(0, ios::beg);
	ofstream os(output_filename, ios::binary);
	if (!os)
		error("Cannot open output file.");
	stream_bit_writer bw(os);
	canonical_huffman_encoder encoder(begin(f), end(f), bw);
	size_t s = 1;
	size_t count = 0;
	vector<uint32_t> keys;
	for (const auto& k : encoder.keys()) {
		if (encoder.at(k).size() == s) {
			++count;
			keys.emplace_back(k);
		}
		else {
			bw.write(count, 8);
			for (auto& v : keys)
				bw.write(v, 24);
			count = 0;
			while (++s != encoder.at(k).size())
				bw.write(count, 8);
			count = 1;
			keys.clear();
			keys.emplace_back(k);
		}
	}
	if (!keys.empty()) {
		bw.write(count, 8);
		for (auto& v : keys)
			bw.write(v, 24);
	}
	bw.write(0xFF, 8);
	size_t acc = 0;
	acc = accumulate(begin(f), end(f), acc, [](size_t acc, const pair<uint32_t, size_t>& p) -> size_t {
		return acc += p.second;
	});
	bw.write(acc, 32);
	read_file(is, encoder);
}

int main(int argc, char **argv) {
	if (argc != 3)
		syntax();

	string input(argv[1]);
	string output(argv[2]);
	encode(input, output);
	cout << "Done!!!\n";

	return EXIT_SUCCESS;
}