#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <cstdint>
#include <unordered_map>
#include "bit.h"

namespace huffman {

	struct huffman_code {
		uint32_t code;
		size_t code_size;

		huffman_code(const uint32_t code_ = 0, const size_t code_size_ = 0);

		huffman_code& operator<<(const size_t n);

		huffman_code& operator+(uint32_t bit);

		void push_back(uint32_t bit);

		huffman_code& operator++();

		bool operator==(const huffman_code& rhs) const;
	};

	struct huffman_code_hash {
		size_t operator()(const huffman_code& code) const;
	};

	class canonical_huffman_decoder {
	public:

		using colors_pair = std::pair<uint8_t, uint8_t>;

		explicit canonical_huffman_decoder(bit::stream_bit_reader& br);

		bool contains(const huffman_code& code) const;

		const colors_pair& at(const huffman_code& code) const;

	private:
		using table_pair = std::pair<colors_pair, uint8_t>;

		std::unordered_map<huffman_code, colors_pair, huffman_code_hash> table_;

		static bool cmp(const table_pair& p1, const table_pair& p2);

	};

}

#endif // HUFFMAN_H