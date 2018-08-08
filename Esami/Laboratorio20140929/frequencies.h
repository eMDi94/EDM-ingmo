#ifndef FREQUENCIES_H
#define FREQUENCIES_H

#include <unordered_map>
#include <iterator>

namespace frequencies {

	template<typename T>
	class frequencies_counter {
	public:
		frequencies_counter() = default;

		void operator()(const T& key) {
			++(table_[key]);
		}

		size_t operator[](const T& key) const {
			return table_.at(key);
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
		std::unordered_map<T, size_t> table_;
	};

}

#endif // FREQUENCIES_H