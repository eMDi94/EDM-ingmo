#ifndef MULTIRES_H
#define MULTIRES_H

#include "core.h"
#include <array>
#include <stdexcept>

namespace multires {

	class _multires_base {
	protected:

		const static core::mat<size_t> adam7_mat;

	public:

		virtual ~_multires_base() = default;
	};

	

	class multires_encoder: public _multires_base {
	private:

		std::array<std::vector<uint8_t>, 7> levels_;

		std::vector<uint8_t>& get(const size_t index) const;

	public:
		explicit multires_encoder(const core::mat<uint8_t>& image);

		std::vector<uint8_t>& operator[](const size_t index);

		const std::vector<uint8_t>& operator[](const size_t index) const;

		auto begin() {
			return std::begin(levels_);
		}

		auto end() {
			return std::end(levels_);
		}

		auto begin() const {
			return std::begin(levels_);
		}

		auto end() const {
			return std::end(levels_);
		}

	};


	class multires_decoder: public _multires_base {
	private:

		static const std::array<size_t, 7> counts;

		size_t height_, width_;
		std::vector<uint8_t> buffer_;
		std::array<size_t, 7> offsets_;

		inline void fill_offsets() {
			std::array<size_t, 7> counts{ 0 };
			for (size_t r = 0; r < height_; ++r) {
				for (size_t c = 0; c < width_; ++c) {
					const size_t level = adam7_mat(r % 8, c % 8);
					++(counts[level - 1]);
				}
			}
			size_t s = 0;
			for (size_t i = 0; i < 7; ++i) {
				offsets_[i] = s;
				s += counts[i];
			}
		}

	public:
		template<typename _InIt>
		multires_decoder(const size_t height, const size_t width, _InIt first, _InIt last): height_(height), width_(width),
														buffer_(height*width), offsets_{ 0 } {
			copy(first, last, begin(buffer_));
			fill_offsets();
		}

		core::mat<uint8_t> decode_level(const size_t level) const {
			if (level < 1 || level > 7)
				throw std::invalid_argument("Level not allowed.");

			core::mat<uint8_t> image(height_, width_);
			std::array<size_t, 7> offsets(offsets_);

			std::array<size_t, 2> sizes{8, 8};
			for (size_t i = 1; i < level; ++i) {
				const size_t r = i % 2;
				sizes[r] = sizes[r] / 2;
			}

			for (size_t r = 0; r < height_; ++r) {
				for (size_t c = 0; c < width_; ++c) {
					const size_t l = adam7_mat(r % 8, c % 8);
					if (l <= level)
						image(r, c) = buffer_[(offsets[l - 1])++];
					else
						image(r, c) = image((r / sizes[0]) * sizes[0], (c / sizes[1]) * sizes[1]);
				}
			}

			return image;
		}
	};

}

#endif // MULTIRES_H