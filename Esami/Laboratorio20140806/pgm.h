#ifndef PGM_H
#define PGM_H

#include "core.h"
#include <iostream>
#include <cstdint>

namespace pgm {
	
	enum class pgm_type { p2, p5 };

	bool read_pgm(std::istream& is, core::mat<uint8_t>& image);
	bool write_pgm(std::ostream& os, const core::mat<uint8_t>& image, const pgm_type type = pgm_type::p5, std::string&& comment = "");

}

#endif // PGM_H