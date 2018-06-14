#ifndef PPM_H
#define PPM_H

#include <iostream>
#include "image.h"

namespace ppm {
	
	enum class ppm_type {p3, p6};

	bool save_ppm(std::ostream& os, const image::mat<image::vec3b>& img,
						ppm_type type = ppm_type::p6, std::string&& comment = "");

}

#endif // PPM_H