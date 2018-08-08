#ifndef PPM_H
#define PPM_H

#include <iostream>
#include "core.h"

namespace ppm {

	enum class ppm_type { p3, p6 };

	bool load_ppm(std::istream& is, core::mat<core::vec3b>& img);
	bool save_ppm(std::ostream& os, const core::mat<core::vec3b>& img,
						ppm_type type = ppm_type::p6, std::string comment = "");

}

#endif // PPM_H