#include "pgm.h"
#include <string>
#include <algorithm>
#include <iterator>


using namespace std;
using namespace core;
using namespace pgm;


bool pgm::read_pgm(std::istream& is, mat<uint8_t>& image) {
	string magic;
	is >> magic;
	if (magic != "P2" && magic != "P5")
		return false;
	is.get();

	if (is.peek() == '#') {
		string comment;
		getline(is, comment);
	}

	size_t width, height, val;
	is >> width >> height >> val;
	if (val != 255)
		return false;
	is.get();

	image.resize(height, width);
	if (magic == "P5") {
		is.unsetf(ios::skipws);
		copy(istream_iterator<uint8_t>(is), istream_iterator<uint8_t>(), begin(image));
	}
	else
		copy(istream_iterator<uint32_t>(is), istream_iterator<uint32_t>(), begin(image));

	return true;
}


bool pgm::write_pgm(ostream& os, const mat<uint8_t>& image, const pgm_type type, string&& comment) {
	if (type == pgm_type::p2)
		os << "P2\n";
	else
		os << "P5\n";

	if (!comment.empty())
		os << "# " << comment << "\n";

	os << image.width() << "\n" << image.height() << "\n" << "255\n";

	if (type == pgm_type::p5)
		copy(begin(image), end(image), ostream_iterator<uint8_t>(os));
	else
		copy(begin(image), end(image), ostream_iterator<uint32_t>(os, " "));

	return true;
}

