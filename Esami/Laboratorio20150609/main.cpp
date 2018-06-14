#include "core.h"
#include "pgm.h"
#include "ppm.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <tuple>

using namespace std;
using namespace core;
using namespace pgm;
using namespace ppm;

void syntax() {
	cerr << "Usage: bayer_decode <input_file>.pgm <output_prefix>";
	exit(EXIT_FAILURE);
}

void error(string error_message) {
	cerr << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

inline bool check_extension(const string& filename, string extension) {
	return filename.substr(filename.size() - extension.size()) == extension;
}

inline uint8_t saturate(double val) {
	return val < 0. ? 0ui8 : val > 255. ? 255ui8 : uint8_t(val);
}

inline uint8_t scale_down(uint16_t val) {
	return saturate(val / 65535. * 255.);
}

inline void write_intermediate_image(const string& output_prefix, const mat<uint8_t>& img) {
	ofstream os(output_prefix + ".pgm", ios::binary);
	if (!os)
		error("Cannot open the output file for intermediate image.");
	if (!save_pgm(os, img))
		error("Cannot save the intermediate image.");
}

inline pair<uint8_t, uint8_t> mean(const mat<vec3b>& img, size_t r, size_t c, size_t v_index, size_t h_index) {
	uint8_t v_mean;
	if (r == 0)
		v_mean = img(r + 1, c)[v_index];
	else
		if (r == img.height() - 1)
			v_mean = img(r - 1, c)[v_index];
		else
			v_mean = round((double(img(r - 1, c)[v_index]) + img(r + 1, c)[v_index]) / 2.);
	
	uint8_t h_mean;
	if (c == 0)
		h_mean = img(r, c + 1)[h_index];
	else
		if (c == img.width() - 1)
			h_mean = img(r, c - 1)[h_index];
		else
			h_mean = round((double(img(r, c - 1)[h_index]) + img(r, c + 1)[h_index]) / 2.);

	return make_pair(v_mean, h_mean);
}

inline tuple<uint16_t, double, double> delta_h(const mat<uint8_t>& img, size_t r, size_t c) {
	int16_t g4 = img(r, c - 1), g6 = img(r, c + 1);
	int16_t x5 = img(r, c), x3 = img(r, c - 2), x7 = img(r, c + 2);

	uint16_t delta = abs(g4 - g6) + abs(2 * x5 - x3 - x7);
	return make_tuple(delta, (g4 + g6) / 2., (2 * x5 - x3 - x7) / 4.);
}

inline tuple<uint16_t, double, double> delta_v(const mat<uint8_t>& img, size_t r, size_t c) {
	int16_t g2 = img(r - 1, c), g8 = img(r + 1, c);
	int16_t x5 = img(r, c), x1 = img(r - 2, c), x9 = img(r + 2, c);

	uint16_t delta = abs(g2 - g8) + abs(2 * x5 - x1 - x9);
	return make_tuple(delta, (g2 + g8) / 2., (2 * x5 - x1 - x9) / 4.);
}


inline uint8_t green_interpolation(const mat<uint8_t>& img, size_t r, size_t c) {
	if (r == 0) {
		if (c == 0)
			return img(r, c + 1);
		else
			if (c == img.width() - 1)
				return img(r, c - 1);
			else
				return saturate((double(img(r, c - 1)) + img(r, c + 1)) / 2.);
	}

	if (r == 1) {
		if (c == img.width() - 1)
			return saturate((double(img(r - 1, c)) + img(r + 1, c)) / 2.);
		else {
			double v_mean = ((double(img(r - 1, c)) + img(r + 1, c)) / 2.);
			double h_mean = ((double(img(r + 1, c)) + img(r + 1, c)) / 2.);
			if (v_mean >= h_mean)
				return saturate(h_mean);
			else
				return saturate(v_mean);
		}
	}

	
	if (r == img.height() - 2) {
		if (r % 2 == 0)
			if (c == 0 || c == img.width() - 1)
				return saturate((double(img(r + 1, c)) + img(r - 1, c)) / 2.);
			else {
				double v_mean = ((double(img(r + 1, c)) + img(r - 1, c)) / 2.);
				double h_mean = ((double(img(r, c + 1)) + img(r, c - 1)) / 2.);
				if (v_mean >= h_mean)
					return saturate(h_mean);
				else
					return saturate(v_mean);
			}
		else
			if (c == img.width() - 1)
				return saturate((double(img(r - 1, c)) + img(r + 1, c)) / 2.);
			else {
				double v_mean = ((double(img(r - 1, c)) + img(r + 1, c)) / 2.);
				double h_mean = ((double(img(r + 1, c)) + img(r + 1, c)) / 2.);
				if (v_mean >= h_mean)
					return saturate(h_mean);
				else
					return saturate(v_mean);
			}
	}

	if (r == img.height() - 1) {
		if (r % 2 == 0)
			if (c == 0)
				return img(r, c + 1);
			else
				if (c == img.width() - 1)
					return img(r, c - 1);
				else
					return saturate((double(img(r, c - 1)) + img(r, c + 1)) / 2.);
		else
			if (c == img.width())
				return img(r, c - 1);
			else
				return saturate((double(img(r, c - 1)) + img(r, c + 1)) / 2.);
	}

	if (c < 2 || c >= img.width() - 2)
		return saturate((double(img(r - 1, c)) + img(r + 1, c)) / 2.);

	const auto dh = delta_h(img, r, c);
	const auto dv = delta_v(img, r, c);

	if (get<0>(dh) > get<0>(dv))
		return saturate(get<1>(dv) + get<2>(dv));
	else
		if (get<0>(dh) < get<0>(dv))
			return saturate(get<1>(dh) + get<2>(dh));
		else
			return saturate(((get<1>(dv) + get<1>(dh)) / 2.) + ((get<2>(dh) + get<2>(dv)) / 2.));
}

inline mat<vec3b>& scan_and_rebuild_green(const mat<uint8_t>& bayer_img, mat<vec3b>& img) {
	img.resize(bayer_img.height(), bayer_img.width());

	for (size_t r = 0; r < img.height(); ++r) {
		for (size_t c = 0; c < img.width(); ++c) {
			if (r % 2 == 0 && c % 2 == 0)
				img(r, c)[0] = bayer_img(r, c);
			if (r % 2 == 1 && c % 2 == 1)
				img(r, c)[2] = bayer_img(r, c);
			if ((r % 2 == 0 && c % 2 == 1) || (r % 2 == 1 && c % 2 == 0))
				img(r, c)[1] = bayer_img(r, c);
			else
				img(r, c)[1] = green_interpolation(bayer_img, r, c);
		}
	}

	return img;
}



inline tuple<uint16_t, double, double> delta_n(const mat<vec3b>& img, size_t r, size_t c, size_t color_index) {
	int16_t g1 = img(r - 1, c - 1)[1], g9 = img(r + 1, c + 1)[1], g5 = img(r, c)[1];
	int16_t x1 = img(r - 1, c - 1)[color_index], x9 = img(r + 1, c + 1)[color_index];

	uint16_t delta = abs(x1 - x9) + abs(2 * g5 - g1 - g9);
	return make_tuple(delta, (x1 + x9) / 2., (2 * g5 - g1 - g9) / 4.);
}

inline tuple<uint16_t, double, double> delta_p(const mat<vec3b>& img, size_t r, size_t c, size_t color_index) {
	int16_t g3 = img(r - 1, c + 1)[1], g5 = img(r, c)[1], g7 = img(r + 1, c - 1)[1];
	int16_t x3 = img(r - 1, c + 1)[color_index], x7 = img(r + 1, c - 1)[color_index];

	uint16_t delta = abs(x3 - x7) + abs(2 * g5 - g3 - g7);
	return make_tuple(delta, (x3 + x7) / 2., (2 * g5 - g3 - g7) / 4.);
}

inline uint8_t x_interpolation(const mat<vec3b>& img, size_t r, size_t c) {
	size_t color_index;
	if (r % 2 == 0)
		color_index = 2;
	else
		color_index = 0;

	if (r == 0) {
		if (c != img.width() - 1)
			return img(r + 1, c - 1)[2];
		else
			return img(r + 1, c + 1)[2];
	}

	if (r == 1) {
		if (c != img.width()) {
			double p_mean = (double(img(r - 1, c - 1)[0]) + img(r + 1, c + 1)[0]) / 2.;
			double n_mean = (double(img(r - 1, c + 1)[0]) + img(r + 1, c - 1)[0]) / 2.;
			if (p_mean >= n_mean)
				return saturate(n_mean);
			else
				return saturate(p_mean);
		}
		else
			return img(r - 1, c - 1)[0];
	}

	if (r == img.height() - 2) {
		if (color_index == 0) {
			if (c != img.width()) {
				double p_mean = (double(img(r - 1, c - 1)[0]) + img(r + 1, c + 1)[0]) / 2.;
				double n_mean = (double(img(r - 1, c + 1)[0]) + img(r + 1, c - 1)[0]) / 2.;
				if (p_mean >= n_mean)
					return saturate(n_mean);
				else
					return saturate(p_mean);
			}
			else
				return img(r - 1, c - 1)[0];
		}
		else {
			if (c == 0)
				return img(r + 1, c + 1)[2];
			else
				if (c == img.width() - 1)
					return img(r + 1, c - 1)[2];
				else {
					double p_mean = (double(img(r - 1, c - 1)[0]) + img(r + 1, c + 1)[0]) / 2.;
					double n_mean = (double(img(r - 1, c + 1)[0]) + img(r + 1, c - 1)[0]) / 2.;
					if (p_mean >= n_mean)
						return saturate(n_mean);
					else
						return saturate(p_mean);
				}
		}
	}

	if (r == img.height() - 1) {
		if (color_index == 0) {
			return img(r - 1, c - 1)[0];
		}
		else {
			if (c != img.width() - 1)
				return img(r - 1, c - 1)[2];
			else
				return img(r - 1, c + 1)[2];
		}
	}

	if (c == 0)
		return img(r + 1, c + 1)[2];

	if (c == 1)
		return img(r - 1, c - 1)[0];

	if (c == img.width() - 2)
		return img(r + 1, c + 1)[color_index];

	if (c == img.width() - 1)
		return img(r - 1, c - 1)[color_index];

	auto dn = delta_n(img, r, c, color_index);
	auto dp = delta_p(img, r, c, color_index);

	if (get<0>(dn) > get<0>(dp))
		return saturate(get<1>(dp) + get<2>(dp));
	else
		if (get<0>(dn) < get<0>(dp))
			return saturate(get<1>(dn) + get<2>(dn));
		else
			return saturate(((get<1>(dn) + get<1>(dp)) / 2.) + ((get<2>(dn) + get<2>(dp)) / 2.));
}

inline mat<vec3b>& rebuild_missing_colors(mat<vec3b>& rgb) {
	for (size_t r = 0; r < rgb.height(); ++r) {
		for (size_t c = 0; c < rgb.width(); ++c) {
			if (r % 2 == 0 && c % 2 == 1) {
				auto p = mean(rgb, r, c, 2, 0);
				vec3b& pixel = rgb(r, c);
				pixel[2] = p.first;
				pixel[0] = p.second;
			}
			if (r % 2 == 1 && c % 2 == 0) {
				auto p = mean(rgb, r, c, 0, 2);
				vec3b& pixel = rgb(r, c);
				pixel[0] = p.first;
				pixel[2] = p.second;
			}
			if (r % 2 == 0 && c % 2 == 0)
				rgb(r, c)[2] = x_interpolation(rgb, r, c);
			if (r % 2 == 1 && c % 2 == 1)
				rgb(r, c)[0] = x_interpolation(rgb, r, c);
		}
	}

	return rgb;
}

void bayer_decode(const string& input_filename, const string& output_prefix) {
	if (!check_extension(input_filename, ".pgm"))
		error("Input file must be a .pgm file.");
	ifstream is(input_filename, ios::binary);
	if (!is)
		error("Cannot open input file.");
	mat<uint16_t> bit16_img;
	if (!load_pgm(is, bit16_img))
		error("Cannot load the input image.");
	mat<uint8_t> bayer_img(bit16_img.height(), bit16_img.width());
	transform(begin(bit16_img), end(bit16_img), begin(bayer_img), scale_down);
	write_intermediate_image(output_prefix, bayer_img);

	mat<vec3b> rgb;
	scan_and_rebuild_green(bayer_img, rgb);
	rebuild_missing_colors(rgb);

	ofstream os(output_prefix + ".ppm", ios::binary);
	if (!os)
		error("Cannot open the output file for final image.");
	if (!save_ppm(os, rgb))
		error("Cannot save the final image.");
}

int main(int argc, char **argv) {
	if (argc != 3)
		syntax();

	string input(argv[1]);
	string output(argv[2]);
	bayer_decode(input, output);
	cout << "Done!!\n";

	return EXIT_SUCCESS;
}