#ifndef PPM_HPP
#define PPM_HPP

#include "core.hpp"
#include <iostream>

namespace ppm {

	// std::istream& operator>>(std::istream& is, core::vec3b& in_vec);
	
	bool load_ppm(std::istream& is, core::mat<core::vec3b>& image);

}

#endif // PPM_HPP