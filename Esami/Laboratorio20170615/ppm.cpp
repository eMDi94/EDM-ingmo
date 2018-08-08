#include "ppm.h"
#include <string>

using namespace std;
using namespace core;
using namespace ppm;

bool ppm::load_ppm(istream& is, mat<vec3b>& img) {
	string magic;
	is >> magic;
	is.get();
	if ((magic != "P3" && magic != "P6") || !is)
		return false;

	if (is.peek() == '#') {
		string comment;
		getline(is, comment);
		if (!is)
			return false;
	}

	size_t width, height;
	uint32_t val;
	is >> width >> height >> val;
	is.get();
	if (val != 255 || !is)
		return false;

	img.resize(height, width);
	if (magic == "P6")
		is.read(reinterpret_cast<char*>(img.data()), height*width * 3);
	else
		for (size_t r = 0; r < height; ++r) {
			for (size_t c = 0; c < width; ++c) {
				vec<uint32_t, 3> pixel;
				is >> ws >> pixel[0] >> pixel[1] >> pixel[2] >> ws;
				img(r, c) = { uint8_t(pixel[0]), uint8_t(pixel[1]), uint8_t(pixel[2]) };
			}
		}

	return true;
}


bool ppm::save_ppm(ostream& os, const mat<vec3b>& img, ppm_type type, string comment) {
	if (type == ppm_type::p6)
		os << "P6\n";
	else
		os << "P3\n";

	if (!comment.empty())
		os << "# " << comment << "\n";

	os << img.width() << " " << img.height() << "\n255\n";

	if (type == ppm_type::p6)
		os.write(reinterpret_cast<const char*>(img.data()), img.width()*img.height() * 3);
	else
		for (size_t r = 0; r < img.height(); ++r) {
			for (size_t c = 0; c < img.width(); ++c) {
				const vec3b& pixel = img(r, c);
				os << uint32_t(pixel[0]) << " " << uint32_t(pixel[1]) << " " << uint32_t(pixel[2]) << " ";
			}
		}

	return os.good();
}