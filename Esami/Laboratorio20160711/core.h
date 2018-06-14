#ifndef CORE_H
#define CORE_H

#include <vector>
#include <iterator>
#include <array>
#include <algorithm>
#include <cstdint>
#include <iostream>

namespace core {
	
	template<typename T>
	class mat {
	public:
		explicit mat(const size_t height = 0, const size_t width = 0): height_(height), width_(width), data_(height*width) {}

		size_t height() const {
			return height_;
		}

		size_t width() const {
			return width_;
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
		vec(Args... args): data_{{ args... }} {}

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
		std::array<T, N> data_;
	};

	template<typename T, size_t N>
	std::istream& operator>>(std::istream& is, vec<T,N>& in_vec) {
		std::copy_n(std::istream_iterator<T>(is), N, begin(in_vec));
		return is;
	}

	template<typename T, size_t N>
	std::ostream& operator<<(std::ostream& os, const vec<T,N>& out_vec) {
		std::copy_n(begin(out_vec), N - 1, std::ostream_iterator<T>(os, " "));
		return os << out_vec[N - 1];
	}

	typedef vec<uint8_t, 3> vec3b;

}

#endif // CORE_H