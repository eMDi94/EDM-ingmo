#include "pgm.h"
#include <iterator>
#include <string>
#include <algorithm>

using namespace std;
using namespace core;
using namespace pgm;

bool pgm::save_pgm(ostream& os, const mat<uint8_t>& img, pgm_type type, string comment) {
	if (type == pgm_type::p5)
		os << "P5\n";
	else
		os << "P2\n";

	if (!comment.empty())
		os << "# " << comment << "\n";

	os << img.width() << " " << img.height() << "\n255\n";

	if (type == pgm_type::p5)
		os.write(reinterpret_cast<const char*>(img.data()), img.height()*img.width());
	else
		copy(begin(img), end(img), ostream_iterator<uint32_t>(os, " "));

	return os.good();
}