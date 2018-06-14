#ifndef PPM_HPP
#define PPM_HPP

#include <iostream>
#include "core.hpp"

namespace ppm {

	bool load_ppm(std::istream& is, core::mat<core::vec3b>& image);

}

#endif // PPM_HPP