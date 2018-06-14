#ifndef MAT_H
#define MAT_H

#include <vector>
#include <iterator>

namespace core {

	template<typename T>
	class mat {
	private:

		size_t height_, width_;
		std::vector<T> data_;

		T& get(const size_t row, const size_t column) const {
			if (row >= height_ || column >= width_)
				throw std::out_of_range("Index of core out of range.");
			return const_cast<T&>(data_[row * width_ + column]);
		}

	public:
		explicit mat(const size_t height = 0, const size_t width = 0):
										height_(height), width_(width), data_(height*width) {}

		// ReSharper disable once CppPossiblyUninitializedMember
		explicit mat(const size_t height, const size_t width, const T* data): mat(height, width) {
			for (size_t i = 0; i < height*width; ++i)
				data_[i] = data[i];
		}

		T& operator()(const size_t row, const size_t column) {
			return get(row, column);
		}

		const T& operator()(const size_t row, const size_t column) const {
			return get(row, column);
		}

		size_t height() const {
			return height_;
		}

		size_t width() const {
			return width_;
		}

		void resize(const size_t new_height, const size_t new_width) {
			height_ = new_height;
			width_ = new_width;
			data_.resize(width_*height_);
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
	};

}

#endif // CORE_H