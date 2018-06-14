#include "ppm.hpp"
#include <algorithm>
#include <string>

using namespace std;
using namespace core;
using namespace ppm;

/*
istream& ppm::operator>>(istream& is, vec3b& in_vec) {
	copy_n(istream_iterator<uint32_t>(is), 3, begin(in_vec));
	return is;
}*/


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

	size_t width, height, value;
	is >> width >> height >> value;
	is.get();
	if (value != 255 || !is)
		return false;

	image.resize(height, width);
	if (magic == "P6")
		is.read(reinterpret_cast<char*>(image.data()), height*width * 3);
	/*
	else
		copy(istream_iterator<vec3b>(is), istream_iterator<vec3b>(), begin(image));
		*/
	return true;
}
