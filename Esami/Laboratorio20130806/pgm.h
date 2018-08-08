#ifndef PGM_H
#define PGM_H

#include <iostream>
#include <cstdint>
#include "core.h"

namespace pgm {

	enum class pgm_type { p2, p5 };

	bool save_pgm(std::ostream& os, const core::mat<uint8_t>& img, pgm_type type = pgm_type::p5, std::string comment = "");

}

#endif // PGM_H