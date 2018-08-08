#include "core.h"
#include "ppm.h"
#include <iostream>
#include <fstream>
#include <string>
#include <iterator>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <array>

using namespace std;
using namespace core;
using namespace ppm;

void syntax() {
	cerr << "Usage: median_cut_reducer <input_filename>.ppm\n";
	exit(EXIT_FAILURE);
}

void error(string error_message) {
	cerr << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

bool check_extension(const string& filename, string extension) {
	return filename.substr(filename.size() - extension.size()) == extension;
}

class box {
public:
	template<typename _Iter>
	explicit box(_Iter first, _Iter last) {
		fill(begin(mins), end(mins), UINT8_MAX);
		fill(begin(maxs), end(maxs), 0ui8);
		for (; first != last; ++first) {
			auto& color = *first;
			for (size_t i = 0; i < color.size(); ++i) {
				if (color[i] < mins[i])
					mins[i] = color[i];
				if (color[i] > maxs[i])
					maxs[i] = color[i];
			}
			colors_.push_back(move(color));
		}
	}

	pair<box, box> split() {
		uint8_t max = 0ui8;
		size_t max_index = 0;
		for (size_t i = 0; i < 3; ++i) {
			uint8_t current = maxs[i] - mins[i];
			if (current > max) {
				max_index = i;
				max = current;
			}
		}

		stable_sort(begin(colors_), end(colors_), [&max_index](const vec3b& c1, const vec3b& c2) -> bool {
			return c1[max_index] < c2[max_index];
		});

		size_t median = colors_.size() / 2;
		auto b = begin(colors_);
		box b1 = box(b, b + median);
		box b2 = box(b + median, end(colors_));

		return make_pair(move(b1), move(b2));
	}

	uint64_t volume() const {
		uint64_t vol = 1;
		for (size_t i = 0; i < 3; ++i)
			vol *= maxs[i] - mins[i];
		return vol;
	}

	vec3b mean() const {
		vec<uint64_t, 3> sums{ 0ui64, 0ui64, 0ui64 };
		for (auto& c : colors_)
			for (size_t i = 0; i < 3; ++i)
				sums[i] += c[i];

		vec3b color;
		for (size_t i = 0; i < 3; ++i) {
			double mean = double(sums[i]) / colors_.size();
			color[i] = mean > 255. ? uint8_t(255) : uint8_t(mean);
		}

		return color;
	}

private:
	array<uint8_t, 3> mins;
	array<uint8_t, 3> maxs;
	vector<vec3b> colors_;
};

void median_cut(const string& input_filename) {
	if (!check_extension(input_filename, ".ppm"))
		error("Input file must be a .ppm file.");
	ifstream is(input_filename, ios::binary);
	if (!is)
		error("Cannot open input file.");

	mat<vec3b> img;
	if (!load_ppm(is, img))
		error("Cannot load input image.");
	mat<vec3b> output(img);

	vector<box> boxes;
	box b(begin(img), end(img));
	boxes.push_back(move(b));
	while (boxes.size() < 64) {
		stable_sort(begin(boxes), end(boxes), [](const box& b1, const box& b2) -> bool {
			return b1.volume() < b2.volume();
		});

		auto& b = boxes.back();
		auto p = b.split();
		boxes.pop_back();
		boxes.push_back(move(p.first));
		boxes.push_back(move(p.second));
	}

	vector<vec3b> colors;
	for (auto& bb : boxes)
		colors.push_back(bb.mean());

	for (auto& c : output) {
		size_t best_index = 0;
		uint32_t error = UINT32_MAX;
		for (size_t i = 0; i < colors.size(); ++i) {
			vec3b& current_color = colors[i];
			int32_t tot = 0;
			for (size_t j = 0; j < 3; ++j)
				tot += int32_t(c[j]) - current_color[j];
			tot = abs(tot);
			if (uint32_t(tot) < error) {
				best_index = i;
				error = tot;
			}
		}
		c = colors[best_index];
	}

	ofstream os("output.ppm", ios::binary);
	if (!os)
		error("Cannot open output file.");
	if (!save_ppm(os, output))
		error("Cannot save the output image.");
}

int main(int argc, char **argv) {
	if (argc != 2)
		syntax();

	string input(argv[1]);
	median_cut(input);
	cout << "Done!!\n";

	return EXIT_SUCCESS;
}