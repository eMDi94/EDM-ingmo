#include "posimage.h"
#include <string>
#include <stdexcept>
#include <iterator>
#include <algorithm>
#include <tuple>

using namespace std;
using namespace pim;

pim_element::pim_element(uint32_t id, size_t id_size, uint64_t size, size_t size_size):
					id_(id), id_size_(id_size), size_(size), s_size_(size_size) {}

uint32_t pim_element::id() const {
	return id_;
}

size_t pim_element::id_size() const {
	return id_size_;
}

size_t pim_element::data_size() const {
	return size_;
}

size_t pim_element::size_size() const {
	return s_size_;
}

size_t pim_element::size() const {
	return id_size_ + s_size_ + size_;
}

const string& pim_element::str_value() const {
	throw runtime_error("Not implemented.");
}

uint64_t pim_element::uint_value() const {
	throw runtime_error("Not implemented.");
}

const vector<uint8_t>& pim_element::binary() const {
	throw runtime_error("Not implemented.");
}

const vector<pim_ptr>& pim_element::elements() const {
	throw runtime_error("Not implemented.");
}

class pim_string : public pim_element {
public:
	explicit pim_string(uint32_t id, size_t id_size, uint64_t size, size_t size_size):
						pim_element(id, id_size, size, size_size) {}

	pim_type type() const override {
		return pim_type::str;
	}

	const string& str_value() const override {
		return str_value_;
	}

protected:

	virtual void read(istream& is) override {
		copy_n(istream_iterator<char>(is), size_, back_inserter(str_value_));
	}

private:
	string str_value_;
};

class pim_uint : public pim_element {
public:
	explicit pim_uint(uint32_t id, size_t id_size, uint64_t size, size_t size_size):
						pim_element(id, id_size, size, size_size), value_(0) {}

	pim_type type() const override {
		return pim_type::uint;
	}

	uint64_t uint_value() const override {
		return value_;
	}

protected:

	void read(istream& is) override {
		uint64_t ret;
		is.read(reinterpret_cast<char*>(&ret), size_);

		for (size_t i = 0; i < size_; ++i, ret = ret >> 8)
			value_ = (value_ << 8) | (ret & 0xFF);
	}

private:
	uint64_t value_;
};

class pim_binary : public pim_element {
public:
	explicit pim_binary(uint32_t id, size_t id_size, uint64_t size, size_t size_size) :
		pim_element(id, id_size, size, size_size), data_(size) {}

	pim_type type() const override {
		return pim_type::binary;
	}

	const vector<uint8_t>& binary() const override {
		return data_;
	}

protected:

	void read(istream& is) override {
		copy_n(istream_iterator<uint8_t>(is), size_, begin(data_));
	}

private:
	vector<uint8_t> data_;
};

class pim_master : public pim_element {
public:
	explicit pim_master(uint32_t id, size_t id_size, uint64_t size, size_t size_size) :
		pim_element(id, id_size, size, size_size) {}

	pim_type type() const override {
		return pim_type::master;
	}

	const vector<pim_ptr>& elements() const override {
		return ptrs_;
	}

protected:

	void read(istream& is) override {
		size_t count = 0;

		while (count != size_) {
			auto e = parse(is);
			count += e->size();
			if (count > size_)
				throw runtime_error("Count exceed size.");
			ptrs_.push_back(move(e));
		}
	}

private:
	vector<pim_ptr> ptrs_;
};


template<typename T>
tuple<T, size_t> read_vint(istream& is) {
	uint8_t first_byte = 0;
	is.read(reinterpret_cast<char*>(&first_byte), 1);

	size_t count = 1;
	uint8_t mask = 0x80;
	for (; (first_byte & mask) == 0; ++count, mask = mask >> 1);

	T ret = first_byte & ((1 << (8 - count)) - 1);
	for (size_t i = 1; i < count; ++i) {
		uint8_t v = 0;
		is.read(reinterpret_cast<char*>(&v), 1);
		ret = (ret << 8) | v;
	}

	return make_tuple(ret, count);
}

pim_ptr pim::parse(istream& is) {
	auto id_t = read_vint<uint32_t>(is);
	auto size_tu = read_vint<uint64_t>(is);
	uint32_t id = get<0>(id_t);
	pim_ptr e;
	switch (id) {
	case 0x0A45DFA3:
		e = make_unique<pim_master>(id, get<1>(id_t), get<0>(size_tu), get<1>(size_tu));
		break;
	case 0x0282:
		e = make_unique<pim_string>(id, get<1>(id_t), get<0>(size_tu), get<1>(size_tu));
		break;
	case 0x0286:
		e = make_unique<pim_uint>(id, get<1>(id_t), get<0>(size_tu), get<1>(size_tu));
		break;
	case 0x20:
		e = make_unique<pim_master>(id, get<1>(id_t), get<0>(size_tu), get<1>(size_tu));
		break;
	case 0x21:
		e = make_unique<pim_uint>(id, get<1>(id_t), get<0>(size_tu), get<1>(size_tu));
		break;
	case 0x22:
		e = make_unique<pim_uint>(id, get<1>(id_t), get<0>(size_tu), get<1>(size_tu));
		break;
	case 0x30:
		e = make_unique<pim_binary>(id, get<1>(id_t), get<0>(size_tu), get<1>(size_tu));
		break;
	default:
		throw runtime_error("Id not recognized.");
	}
	e->read(is);
	return e;
}

vector<pim_ptr> pim::parse_multi(istream& is) {
	vector<pim_ptr> ptrs;
	while (is.peek() != EOF)
		ptrs.emplace_back(parse(is));
	return ptrs;
}