#ifndef PGM_H
#define PGM_H

#include <iostream>
#include "image.h"
#include <cstdint>

namespace pgm {
	
	enum class pgm_type {p2, p5};

	bool save_pgm(std::ostream& os, const image::mat<uint8_t>& img,
					pgm_type type = pgm_type::p5, std::string&& comment = "");

}

#endif // PGM_H