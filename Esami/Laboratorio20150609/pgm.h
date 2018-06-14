#ifndef PGM_H
#define PGM_H

#include "core.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <iterator>

namespace pgm {

	template<typename T>
	bool load_pgm(std::istream& is, core::mat<T>& img) {
		std::string magic;
		is >> magic;
		is.get();
		if ((magic != "P2" && magic != "P5") || !is)
			return false;

		if (is.peek() == '#') {
			std::string comment;
			getline(is, comment);
		}

		size_t width, height;
		uint32_t value;
		is >> width >> height >> value;
		is.get();
		if (!is)
			return false;
		if (value > 255 && sizeof(T) == 1)
			return false;

		img.resize(height, width);
		if (magic == "P5") {
			is.read(reinterpret_cast<char*>(img.data()), height*width * sizeof(T));
			if (sizeof(T) > 1)
				std::transform(std::begin(img), std::end(img), std::begin(img), [](T val) -> T {
				T ret = 0;
				for (size_t i = 0; i < sizeof(T); ++i) {
					ret = (ret << 8) | (val & 0xFF);
					val = val >> 8;
				}
				return ret;
			});
		}
		else
			if (sizeof(T) == 1)
				std::copy(std::istream_iterator<uint32_t>(is), std::istream_iterator<uint32_t>(), std::begin(img));
			else
				std::copy(std::istream_iterator<T>(is), std::istream_iterator<T>(), std::begin(img));

		return true;
	}

	enum class pgm_type {p2, p5};

	template<typename T>
	bool save_pgm(std::ostream& os, const core::mat<T>& img, pgm_type type = pgm_type::p5, std::string comment = "") {
		if (type == pgm_type::p5)
			os << "P5\n";
		else
			os << "P2\n";

		if (!comment.empty())
			os << "# " << comment << "\n";

		os << img.width() << " " << img.height() << "\n";
		if (sizeof(T) == 1)
			os << "255\n";
		else
			os << "65535\n";

		if (type == pgm_type::p5) {
			if (sizeof(T) == 1)
				os.write(reinterpret_cast<const char*>(img.data()), img.height()*img.width());
			else
				for (auto val : img) {
					T ret = 0;
					for (size_t i = 0; i < sizeof(T); ++i) {
						ret = (ret << 8) | (val & 0xFF);
						val = val >> 8;
					}
					os.write(reinterpret_cast<const char*>(&ret), sizeof(T));
				}
		}
		else {
			if (sizeof(T) == 1)
				std::copy(std::begin(img), std::end(img), std::ostream_iterator<uint32_t>(os, " "));
			else
				std::copy(std::begin(img), std::end(img), std::ostream_iterator<T>(os, " "));
		}

		return os.good();
	}

}

#endif // PGM_H