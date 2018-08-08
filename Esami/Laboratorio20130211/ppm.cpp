#include "ppm.h"
#include <string>

using namespace std;
using namespace core;
using namespace ppm;

bool ppm::save_ppm(ostream& os, const mat<vec3b>& img, ppm_type type, string comment) {
	if (type == ppm_type::p3)
		os << "P3\n";
	else
		os << "P6\n";

	if (!comment.empty())
		os << "# " << comment << "\n";

	os << img.width() << " " << img.height() << "\n255\n";

	if (type == ppm_type::p3)
		for (auto it = begin(img); it != end(img); ++it) {
			const auto& pixel = *it;
			os << uint32_t(pixel[0]) << " " << uint32_t(pixel[1]) << " " << uint32_t(pixel[2]) << " ";
		}
	else
		os.write(reinterpret_cast<const char*>(img.data()), img.width()*img.height() * 3);

	return os.good();
}