#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <vector>
#include <iostream>
#include <unordered_map>
#include <cstdint>
#include <stdexcept>
#include "bit.h"
#include <string>
#include <algorithm>
#include <iterator>

namespace huffman {

	typedef bool bit;

	class huffman_code {
	public:
		huffman_code(bit bit_value = 0) : code_(), value_(bit_value) {}

		huffman_code(size_t n, bit initial_bit = 0) : code_(n) {}

		size_t size() const {
			return code_.size();
		}

		bit operator[](size_t index) const {
			return code_[index];
		}

		void set(size_t index, bit value) {
			code_[index] = value;
		}

		huffman_code& operator<<(size_t s) {
			while (s-- > 0)
				code_.push_back(value_);
			return *this;
		}

		void insert_front(bit b) {
			code_.insert(std::begin(code_), b);
		}

		void insert_back(bit b) {
			code_.emplace_back(b);
		}

		huffman_code& operator+(bit b) {
			if (b == 0)
				return *this;
			
			size_t i;
			for (i = size(); i > 0 && b != 0; --i) {
				if ((*this)[i - 1] == 1) {
					set(i - 1, 0);
				}
				else {
					set(i - 1, 1);
					b = 0;
				}
			}
			/*if (b == 1)
				throw std::overflow_error("Sum causes overflow.");*/

			return *this;
		}

		huffman_code& operator++() {
			return *this + 1;
		}

		bool operator==(const huffman_code& rhs) const {
			return code_ == rhs.code_;
		}

	private:
		std::vector<bool> code_;
		bit value_;

		friend struct huffman_code_hash;
	};

	struct huffman_code_hash {
		size_t operator()(const huffman_code& rhs) const {
			std::hash<std::vector<bool>> h;
			return h(rhs.code_);
		}
	};

	class canonical_huffman_decoder {
	public:
		explicit canonical_huffman_decoder(bit::stream_bit_reader& br) {
			uint16_t n = br.get<uint16_t>(16);
			std::vector<table_p> elems;
			elems.reserve(n);
			for (uint16_t i = 0; i < n; ++i) {
				std::string str;
				std::getline(br.is, str, '\0');
				uint8_t len;
				br.is.read(reinterpret_cast<char*>(&len), 1);
				elems.emplace_back(std::make_pair(std::move(str), len));
			}

			std::stable_sort(std::begin(elems), std::end(elems), canonical_huffman_decoder::cmp);

			huffman_code code;
			for (auto& p : elems) {
				if (code.size() < p.second)
					code << p.second - code.size();
				table_.insert(std::make_pair(code, std::move(p.first)));
				++code;
			}
		}

		const std::string& at(const huffman_code& code) const {
			return table_.at(code);
		}

		bool contains(const huffman_code& code) const {
			return table_.find(code) != std::end(table_);
		}

	private:
		using table_p = std::pair<std::string, uint8_t>;
		
		std::unordered_map<huffman_code, std::string, huffman_code_hash> table_;

		static bool cmp(const table_p& p1, const table_p& p2) {
			return p1.second < p2.second;
		}
	};

}

#endif // HUFFMAN_H