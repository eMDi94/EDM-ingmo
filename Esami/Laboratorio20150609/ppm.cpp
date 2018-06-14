#include "ppm.h"
#include <string>

using namespace std;
using namespace core;
using namespace ppm;

bool ppm::save_ppm(ostream& os, const mat<vec3b>& img, ppm_type type, string comment) {
	if (type == ppm_type::p6)
		os << "P6\n";
	else
		os << "P3\n";

	if (!comment.empty())
		os << "# " << comment << "\n";

	os << img.width() << " " << img.height() << "\n255\n";

	if (type == ppm_type::p6)
		os.write(reinterpret_cast<const char*>(img.data()), img.height()*img.width() * 3);
	else
		for (const auto& pixel : img)
			os << pixel[0] << " " << pixel[1] << " " << pixel[2] << " ";

	return os.good();
}

bool ppm::read_ppm(istream& is, mat<vec3b>& img) {
	string magic;
	is >> magic;
	is.get();
	if ((magic != "P6" && magic != "P3") || !is)
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
	is.read(reinterpret_cast<char*>(img.data()), height*width * 3);

	return true;
}