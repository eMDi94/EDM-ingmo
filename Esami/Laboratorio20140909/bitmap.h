#ifndef BITMAP_H
#define BITMAP_H

#include <iostream>
#include "core.h"

namespace bmp {

	bool load_bmp(std::istream& is, core::mat<core::vec3b>& img);

}

#endif // BITMAP_H