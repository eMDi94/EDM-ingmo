#ifndef BIT_H
#define BIT_H

#include <iostream>
#include <cstdint>

namespace bit {

	class stream_bit_reader {
	public:
		explicit stream_bit_reader(std::istream& is) : is_(is), buffer_(0), count_(0) {}

		template<typename T>
		T get(size_t n = sizeof(T) * 8) {
			T val = 0;
			while (n-- > 0)
				val = (val << 1) | read_bit();
			return val;
		}

		bool eof() {
			return is_.eof();
		}

	private:
		std::istream& is_;
		uint8_t buffer_;
		uint8_t count_;

		uint8_t read_bit() {
			if (count_ == 0) {
				is_.read(reinterpret_cast<char*>(&buffer_), 1);
				count_ = 8;
			}

			return (buffer_ >> (--count_)) & 0x01;
		}
	};
}

#endif // BIT_H