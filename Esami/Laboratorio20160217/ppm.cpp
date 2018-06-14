#include "ppm.h"
#include <iterator>
#include <string>

using namespace std;
using namespace image;
using namespace ppm;


bool ppm::save_ppm(ostream& os, const mat<vec3b>& img, ppm_type type, string&& comment) {
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
		for (const auto& out : img)
			os << out[0] << " " << out[1] << " " << out[2] << " ";

	return os.good();
}
