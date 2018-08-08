#ifndef PPM_H
#define PPM_H

#include "core.h"
#include <iostream>

namespace ppm {

	enum class ppm_type {p3, p6};

	bool save_ppm(std::ostream& os, const core::mat<core::vec3b>& img,
				const ppm_type type = ppm_type::p6, const std::string comment = "");

}

#endif // PPM_H