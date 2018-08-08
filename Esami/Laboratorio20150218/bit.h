#ifndef BIT_H
#define BIT_H

#include <cstdint>
#include <iostream>

namespace bit {

	class stream_bit_reader {
	public:
		std::istream& is;

		explicit stream_bit_reader(std::istream& is) : is(is), buffer_(0), count_(0) {}

		template<typename T>
		T get(size_t n) {
			T val = 0;
			while (n-- > 0)
				val = (val << 1) | read_bit();
			return val;
		}

	private:
		uint8_t buffer_;
		size_t count_;

		uint8_t read_bit() {
			if (count_ == 0) {
				is.read(reinterpret_cast<char*>(&buffer_), 1);
				count_ = 8;
			}
			return (buffer_ >> (--count_)) & 0x01;
		}
	};

} 

#endif  // BIT_H