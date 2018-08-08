#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <vector>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include "bit.h"
#include <iterator>

namespace huffman {

	typedef bool bit;

	class huffman_code {
	public:
		huffman_code() = default;

		size_t size() const {
			return code_.size();
		}

		void set(size_t n, bit b) {
			code_[n] = b;
		}

		bit operator[](size_t n) const {
			return code_[n];
		}

		void push_back(bit b) {
			code_.push_back(b);
		}

		bit back() const {
			return code_.back();
		}

		bit pop_back() {
			bit b = back();
			code_.pop_back();
			return b;
		}

		huffman_code& operator<<(size_t n) {
			while (n-- > 0)
				push_back(0);
			return *this;
		}

		huffman_code& operator+(bit b) {
			if (b != 0)
				for (size_t i = size(); i > 0 && b != 0; --i)
					if ((*this)[i - 1] == 0) {
						set(i - 1, 1);
						b = 0;
					}
					else
						set(i - 1, 0);
			
			return *this;
		}

		huffman_code& operator++() {
			return *this + 1;
		}

		auto begin() {
			return std::begin(code_);
		}

		auto end() {
			return std::end(code_);
		}

		auto begin() const {
			return std::begin(code_);
		}

		auto end() const {
			return std::end(code_);
		}

	private:
		std::vector<bool> code_;
	};


	class canonical_huffman_encoder {
	public:
		template<typename _InIt>
		canonical_huffman_encoder(_InIt first, _InIt last, bit::stream_bit_writer& bw): table_(), bw_(bw), oredered_keys_() {
			std::priority_queue<node, std::vector<node>, decltype(&canonical_huffman_encoder::pq_cmp)>
				pq(canonical_huffman_encoder::pq_cmp);
			for (; first != last; ++first) {
				auto p = *first;
				std::vector<uint32_t> keys{ p.first };
				pq.emplace(std::move(keys), p.second);
			}

			std::unordered_map<uint32_t, size_t> sizes;
			while (pq.size() > 1) {
				auto n1 = pq.top();
				pq.pop();
				auto n2 = pq.top();
				pq.pop();
				std::vector<uint32_t> keys(std::begin(n1.first), std::end(n1.first));
				std::copy(std::begin(n2.first), std::end(n2.first), std::back_inserter(keys));
				for (auto k : keys)
					++(sizes[k]);
				pq.emplace(std::move(keys), n1.second + n2.second);
			}

			std::vector<vpair> buffer(std::begin(sizes), std::end(sizes));
			std::stable_sort(std::begin(buffer), std::end(buffer), canonical_huffman_encoder::ss_cmp);
			if (buffer.back().second > 254)
				throw std::domain_error("Max code size is 254.");

			huffman_code code;
			for (auto& k : buffer) {
				if (code.size() < k.second)
					code << k.second - code.size();
				table_.emplace(k.first, code);
				++code;
				oredered_keys_.emplace_back(k.first);
			}
		}

		const huffman_code& at(const uint32_t key) const {
			return table_.at(key);
		}

		void operator()(const uint32_t key) const {
			const huffman_code& code = table_.at(key);
			bw_.write(code);
		}

		size_t size() const {
			return table_.size();
		}

		const std::vector<uint32_t>& keys() const {
			return oredered_keys_;
		}

		auto begin() {
			return std::begin(table_);
		}

		auto end() {
			return std::end(table_);
		}

		auto begin() const {
			return std::begin(table_);
		}

		auto end() const {
			return std::end(table_);
		}

	private:
		std::unordered_map<uint32_t, huffman_code> table_;
		std::vector<uint32_t> oredered_keys_;
		bit::stream_bit_writer& bw_;

		using node = std::pair<std::vector<uint32_t>, size_t>;

		using vpair = std::pair<uint32_t, size_t>;

		static bool pq_cmp(node& p1, node& p2) {
			return p1.second > p2.second;
		}

		static bool ss_cmp(const vpair& p1, const vpair& p2) {
			if (p1.second == p2.second)
				return p1.first < p2.first;
			return p1.second < p2.second;
		}
	
	};

}

#endif // HUFFMAN_H