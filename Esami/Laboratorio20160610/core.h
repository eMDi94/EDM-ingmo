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
		explicit mat(const size_t height, const size_t width) : height_(height), width_(width), data_(height*width) {}

		size_t height() const {
			return height_;
		}

		size_t width() const {
			return width_;
		}

		void resize(size_t new_height, size_t new_width) {
			height_ = new_height;
			width_ = new_width;
			data_.resize(height_*width_);
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

	template<typename T, size_t N>
	class vec {
	public:
		template<typename... Args>
		vec(Args... args) : data_{ {args...} } {}

		size_t size() const {
			return data_.size();
		}

		T& operator[](const size_t index) {
			return data_[index];
		}

		const T& operator[](const size_t index) const {
			return data_[index];
		}

		T* data() {
			return data_.data();
		}

		const T* data() const {
			return data_.data();
		}

	private:
		std::array<T, N> data_;
	};

	typedef vec<uint8_t, 3> vec3b;
}

#endif // CORE_H