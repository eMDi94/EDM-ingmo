#include "bitmap.h"
#include <map>

using namespace std;
using namespace core;

typedef vec<uint8_t, 4> vec4b;
typedef map<uint8_t, vec4b> color_table;


bool load24(istream& is, mat<vec3b>& img) {
	for (size_t r = img.height(); r > 0; --r) {
		size_t readed_bit = 0;
		for (size_t c = 0; c < img.width(); ++c) {
			vec3b bgr;
			is.read(reinterpret_cast<char*>(bgr.data()), 3);
			readed_bit += 24;
			img(r - 1, c) = { bgr[2], bgr[1], bgr[0] };
		}
		size_t rm = readed_bit % 32;
		if (rm != 0)
			is.seekg((32 - rm) / 8, ios::cur);
	}
	return true;
}

inline color_table load_table(istream& is, size_t n) {
	color_table table;
	for (size_t i = 0; i < n; ++i) {
		vec4b bgr0;
		is.read(reinterpret_cast<char*>(bgr0.data()), 4);
		table.insert(make_pair(i, move(bgr0)));
	}
	return table;
}

bool load8(istream& is, mat<vec3b>& img, size_t color_table_size) {
	auto table = load_table(is, color_table_size);
	for (size_t r = img.height(); r > 0; --r) {
		size_t readed_bit = 0;
		for (size_t c = 0; c < img.width(); ++c) {
			uint8_t index;
			is.read(reinterpret_cast<char*>(&index), 1);
			readed_bit += 8;
			vec4b& bgr0 = table.at(index);
			img(r - 1, c) = { bgr0[2], bgr0[1], bgr0[0] };
		}
		size_t rm = readed_bit % 32;
		if (rm != 0)
			is.seekg((32 - rm) / 8, ios::cur);
	}
	return true;
}

bool load4(istream& is, mat<vec3b>& img, size_t color_table_size) {
	auto table = load_table(is, color_table_size);
	for (size_t r = img.height(); r > 0; --r) {
		size_t readed_bit = 0;
		for (size_t c = 0; c < img.width();) {
			uint8_t val;
			is.read(reinterpret_cast<char*>(&val), 1);
			readed_bit += 8;
			for (uint8_t mask = 0xF0, i = 2; c < img.width() && i > 0; ++c, mask = mask >> 4, --i) {
				uint8_t index = (val & mask) >> ((i - 1) * 4);
				vec4b& bgr0 = table.at(index);
				img(r - 1, c) = { bgr0[2], bgr0[1], bgr0[0] };
			}
		}
		size_t rm = readed_bit % 32;
		if (rm != 0)
			is.seekg((32 - rm) / 8, ios::cur);
	}
	return true;
}

bool load1(istream& is, mat<vec3b>& img, size_t color_table_size) {
	auto table = load_table(is, color_table_size);
	for (size_t r = img.height(); r > 0; --r) {
		size_t readed_bit = 0;
		for (size_t c = 0; c < img.width();) {
			uint8_t val;
			is.read(reinterpret_cast<char*>(&val), 1);
			readed_bit += 8;
			for (uint8_t mask = 0x80, i = 8; c < img.width() && i > 0; mask = mask >> 1, --i, ++c) {
				uint8_t index = (val & mask) >> (i - 1);
				vec4b& bgr0 = table.at(index);
				img(r - 1, c) = { bgr0[2], bgr0[1], bgr0[0] };
			}
		}
		size_t rm = readed_bit % 32;
		if (rm != 0)
			is.seekg((32 - rm) / 8, ios::cur);
	}
	return true;
}

bool bmp::load_bmp(istream& is, mat<vec3b>& img) {
	char header[54];
	is.read(header, 54);
	if (header[0] != 0x42 && header[1] != 0x4D)
		return false;
	int32_t width = *(reinterpret_cast<int32_t*>(header + 18));
	int32_t height = *(reinterpret_cast<int32_t*>(header + 22));
	if (*(reinterpret_cast<uint16_t*>(header + 26)) != 1)
		return false;
	uint16_t bpp = *(reinterpret_cast<uint16_t*>(header + 28));
	size_t palette_size = *(reinterpret_cast<size_t*>(header + 46));
	if (palette_size == 0)
		palette_size = size_t(1) << bpp;
	img.resize(height, width);

	switch (bpp) {
	case 24:
		return load24(is, img);
	case 8:
		return load8(is, img, palette_size);
	case 4:
		return load4(is, img, palette_size);
	case 1:
		return load1(is, img, palette_size);
	default:
		return false;
	}
}