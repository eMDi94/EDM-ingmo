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
		os.write(reinterpret_cast<const char*>(img.data()), img.width()*img.height() * 3);
	else
		for (const auto& pixel : img)
			os << uint32_t(pixel[0]) << " " << uint32_t(pixel[1]) << " " << uint32_t(pixel[2]) << " ";

	return os.good();
}