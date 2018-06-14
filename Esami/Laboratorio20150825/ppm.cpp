#include "ppm.hpp"
#include <string>
#include <algorithm>
#include <iterator>

using namespace std;
using namespace core;
using namespace ppm;


istream& operator>>(istream& is, vec3b& in_vec) {
	uint32_t val;
	for (size_t i = 0; i < 3; ++i) {
		is >> val;
		in_vec[i] = uint8_t(val);
	}
	return is;
}


bool ppm::load_ppm(istream& is, mat<vec3b>& image) {
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

	size_t height, width;
	uint32_t value;
	is >> width >> height >> value;
	is.get();
	if (!is || value != 255)
		return false;

	image.resize(height, width);

	if (magic == "P6")
		is.read(reinterpret_cast<char*>(image.data()), height*width * 3);
	else
		copy(istream_iterator<vec3b>(is), istream_iterator<vec3b>(), begin(image));

	return true;
}
