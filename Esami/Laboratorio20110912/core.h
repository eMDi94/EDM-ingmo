#ifndef CORE_H
#define CORE_H

#include <vector>
#include <iterator>
#include <array>
#include <cstdint>

namespace core {

	template<typename T>
	class mat {
	public:
		explicit mat(size_t height = 0, size_t width = 0) : height_(height), width_(width), data_(height*width) {}

		size_t height() const {
			return height_;
		}

		size_t width() const {
			return width_;
		}

		void resize(size_t nheight, size_t nwidth) {
			height_ = nheight;
			width_ = nwidth;
			data_.resize(height_*width_);
		}

		T& operator()(size_t r, size_t c) {
			return data_[r * width_ + c];
		}

		const T& operator()(size_t r, size_t c) const {
			return data_[r * width_ + c];
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

	template<typename T, size_t N>
	class vec {
	public:
		template<typename... Args>
		vec(Args... args) : data_{ {args...} } {}

		size_t size() const {
			return data_.size();
		}

		T& operator[](size_t i) {
			return data_[i];
		}

		const T& operator[](size_t i) const {
			return data_[i];
		}

	private:
		std::array<T, N> data_;
	};

	typedef vec<uint8_t, 3> vec3b;
}

#endif // CORE_H