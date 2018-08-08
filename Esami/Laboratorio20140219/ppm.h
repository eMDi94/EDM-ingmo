#ifndef PPM_H
#define PPM_H

#include "core.h"
#include <iostream>

namespace ppm {
	
	enum class ppm_type { p6, p3 };

	bool save_ppm(std::ostream& os, const core::mat<core::vec3b>& image, const ppm_type type = ppm_type::p6,
					std::string&& comment = "");

}

#endif // PPM_H