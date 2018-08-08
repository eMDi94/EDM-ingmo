#ifndef BIT_H
#define BIT_H

#include <cstdint>
#include <iostream>

namespace bit {

	class stream_bit_writer {
	public:
		explicit stream_bit_writer(std::ostream& os) : os_(os), buffer_(0), count_(8) {}

		template<typename T>
		void write(const T val, size_t n) {
			while (n-- > 0)
				write_bit(val >> n);
		}

		template<typename Code>
		void write(Code code) {
			for (size_t i = 0; i < code.size(); ++i)
				write_bit(code[i]);
		}

		void flush(uint8_t val = 1) {
			while (count_ != 8)
				write_bit(val);
		}

		~stream_bit_writer() {
			flush();
		}

	private:
		std::ostream& os_;
		uint8_t buffer_, count_;
	
		void write_bit(uint8_t bit) {
			buffer_ = buffer_ | (bit << (--count_));

			if (count_ == 0) {
				os_.write(reinterpret_cast<const char*>(&buffer_), 1);
				buffer_ = 0;
				count_ = 8;
			}
		}
	};

}

#endif // BIT_H