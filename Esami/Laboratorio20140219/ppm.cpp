#include "ppm.h"
#include <algorithm>
#include <string>

using namespace std;
using namespace core;
using namespace ppm;


bool ppm::save_ppm(ostream& os, const mat<vec3b>& image, const ppm_type type, string&& comment) {
	if (type == ppm_type::p6)
		os << "P6\n";
	else
		os << "P3\n";

	os << "# " << comment << "\n" << image.width() << " " << image.height() << "\n255\n";

	if (type == ppm_type::p6)
		os.write(reinterpret_cast<const char*>(image.data()), image.width()*image.height() * 3);
	else
		copy(begin(image), end(image), ostream_iterator<vec3b>(os, " "));

	return os.good();
}
