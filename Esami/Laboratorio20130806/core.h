#ifndef CORE_H
#define CORE_H

#include <vector>
#include <iterator>

namespace core {

	template<typename T>
	class mat {
	public:
		explicit mat(const size_t height = 0, const size_t width = 0) :
			height_(height), width_(width), data_(height*width) {}

		size_t height() const {
			return height_;
		}

		size_t width() const {
			return width_;
		}

		void resize(const size_t nheight, const size_t nwidth) {
			height_ = nheight;
			width_ = nwidth;
			data_.resize(nheight*nwidth);
		}

		T& operator()(const size_t row, const size_t column) {
			return data_[row * width_ + column];
		}

		const T& operator()(const size_t row, const size_t column) const {
			return data_[row * width_ + column];
		}

		T* data() {
			return data_.data();
		}

		const T* data() const {
			return data_.data();
		}

		auto begin() {
			return std::begin(data_);
		}

		auto end() {
			return std::end(data_);
		}

		auto begin() const {
			return std::begin(data_);
		}

		auto end() const {
			return std::end(data_);
		}

	private:
		size_t height_, width_;
		std::vector<T> data_;
	};

}

#endif // CORE_H