#include "huffman.h"
#include <algorithm>
#include <vector>
#include <stdexcept>

using namespace std;
using namespace bit;
using namespace huffman;

huffman_code::huffman_code(const uint32_t code_, const size_t code_size_) : code(code_), code_size(code_size_) {}

huffman_code& huffman_code::operator<<(const size_t n) {
	code = code << n;
	code_size += n;
	return *this;
}

huffman_code& huffman_code::operator+(uint32_t bit) {
	if (bit != 0 && bit != 1)
		throw invalid_argument("Only 0 and 1 are allowed.");
	code += bit;
}

void huffman_code::push_back(uint32_t bit) {
	code = code << 1;
	code |= bit & 0x01;
	++code_size;
}

huffman_code& huffman_code::operator++() {
	return *this + 1;
}

bool huffman_code::operator==(const huffman_code& rhs) const {
	return code_size == rhs.code_size ? code == rhs.code : false;
}


size_t huffman_code_hash::operator()(const huffman_code& code) const {
	std::hash<uint32_t> h;
	return h(code.code);
}

canonical_huffman_decoder::canonical_huffman_decoder(stream_bit_reader& br) {
	uint16_t num_element = br.get<uint16_t>();
	vector<table_pair> elems;
	elems.reserve(num_element);
	for (uint16_t i = 0; i < num_element; ++i) {
		uint8_t r = br.get<uint8_t>();
		uint8_t g_b = br.get<uint8_t>();
		uint8_t code_size = br.get<uint8_t>(5);
		auto p = make_pair(make_pair(r, g_b), code_size);
		elems.emplace_back(p);
	}

	stable_sort(std::begin(elems), std::end(elems), canonical_huffman_decoder::cmp);

	huffman_code code;
	for (auto& p : elems) {
		if (code.code_size < p.second)
			code << p.second - code.code_size;
		table_.insert(std::make_pair(code, std::move(p.first)));
		++code;
	}
}

bool canonical_huffman_decoder::contains(const huffman_code& code) const {
	auto f = table_.find(code);
	auto e = end(table_);
	return f != e;
}

const canonical_huffman_decoder::colors_pair& canonical_huffman_decoder::at(const huffman_code& code) const {
	return table_.at(code);
}

bool canonical_huffman_decoder::cmp(const table_pair& p1, const table_pair& p2) {
	return p1.second < p2.second;
}