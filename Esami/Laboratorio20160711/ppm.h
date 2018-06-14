#ifndef PPM_H
#define PPM_H

#include "core.h"

namespace ppm {
	
	enum class ppm_type{p3, p6};

	bool save_ppm(std::ostream& os, const core::mat<core::vec3b>& image, ppm_type type = ppm_type::p6,
					std::string&& comment = "");

}

#endif // PPM_H