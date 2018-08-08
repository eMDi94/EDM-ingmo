#include "ubjson.h"
#include <stdexcept>
#include <iterator>
#include <algorithm>

using namespace std;
using namespace ubjson;

bool ubjson::ubjson_value::boolean() const {
	throw domain_error("Not implemented.");
}

int8_t ubjson::ubjson_value::int8() const
{
	throw domain_error("Not implemented");
}

uint8_t ubjson::ubjson_value::uint8() const
{
	throw domain_error("Not implemented");
}

int16_t ubjson::ubjson_value::int16() const
{
	throw domain_error("Not implemented");
}

int32_t ubjson::ubjson_value::int32() const
{
	throw domain_error("Not implemented");
}

float ubjson::ubjson_value::float32() const
{
	throw domain_error("Not implemented");
}

double ubjson::ubjson_value::float64() const
{
	throw domain_error("Not implemented");
}

char ubjson::ubjson_value::character() const
{
	throw domain_error("Not implemented");
}

const std::string & ubjson::ubjson_value::str() const
{
	throw domain_error("Not implemented");
}

const ubjson_array & ubjson::ubjson_value::ubj_array() const
{
	throw domain_error("Not implemented");
}

const object & ubjson::ubjson_value::obj() const
{
	throw domain_error("Not implemented");
}

int64_t ubjson::ubjson_value::int_value() const
{
	switch (type()) {
	case vtype::int8:
		return int8();
	case vtype::uint8:
		return uint8();
	case vtype::int16:
		return int16();
	case vtype::int32:
		return int32();
	default:
		throw domain_error("Not an int.");
	}
}


class null_value : public ubjson_value {
public:
	vtype type() const override {
		return vtype::null;
	}

	void read(istream& is) override {
		if (is.get() != 'Z')
			throw domain_error("Not a null.");
	}
};

class noop_value : public ubjson_value {
public:
	vtype type() const override {
		return vtype::noop;
	}

	void read(istream& is) override {
		if (is.get() != 'N')
			throw domain_error("Not a noop.");
	}
};

class boolean_value : public ubjson_value {
public:
	vtype type() const override {
		return vtype::boolean;
	}

	void read(istream& is) override {
		switch (is.get()) {
		case 'T':
			val_ = true;
			break;
		case 'F':
			val_ = false;
			break;
		default:
			throw domain_error("Not a bool.");
		}
	}

	bool boolean() const override {
		return val_;
	}

private:
	bool val_;
};

template<typename T>
T flip(T val, size_t size = sizeof(T)) {
	T ret = 0;
	for (size_t i = 0; i < size; ++i, val = val >> 8)
		ret = (ret << 8) | (val & 0xFF);
	return ret;
}

class int8_value : public ubjson_value {
public:
	vtype type() const override {
		return vtype::int8;
	}

	void read(istream& is) override {
		if (is.get() != 'i')
			throw domain_error("Not an int8.");
		val_ = is.get();
	}

	int8_t int8() const override {
		return val_;
	}

private:
	int8_t val_;
};

class uint8_value : public ubjson_value {
public:
	uint8_value() = default;

	uint8_value(uint8_t& v) : val_(v) {}

	vtype type() const override {
		return vtype::uint8;
	}

	void read(istream& is) override {
		if (is.get() != 'U')
			throw domain_error("Not an uint8.");
		is.read(reinterpret_cast<char*>(&val_), 1);
	}

	uint8_t uint8() const override {
		return val_;
	}

private:
	uint8_t val_;
};

class int16_value : public ubjson_value {
public:
	vtype type() const override {
		return vtype::int16;
	}

	void read(istream& is) override {
		if (is.get() != 'I')
			throw domain_error("Not an int16.");
		is.read(reinterpret_cast<char*>(&val_), 2);
		val_ = flip(val_);
	}

	int16_t int16() const override {
		return val_;
	}

private:
	int16_t val_;
};

class int32_value : public ubjson_value {
public:
	vtype type() const override {
		return vtype::int32;
	}

	void read(istream& is) override {
		if (is.get() != 'l')
			throw domain_error("Not an int32.");
		is.read(reinterpret_cast<char*>(&val_), 4);
		val_ = flip(val_);
	}

	int32_t int32() const override {
		return val_;
	}

private:
	int32_t val_;
};

class float32_value : public ubjson_value {
public:
	vtype type() const override {
		return vtype::float32;
	}

	void read(istream& is) override {
		if (is.get() != 'd')
			throw domain_error("Not a float32.");
		is.read(reinterpret_cast<char*>(&val_), 4);
	}

	float float32() const override {
		return val_;
	}

private:
	float val_;
};

class float64_value : public ubjson_value {
public:
	vtype type() const override {
		return vtype::float64;
	}

	void read(istream& is) override {
		if (is.get() != 'D')
			throw domain_error("Not a float64");
		is.read(reinterpret_cast<char*>(&val_), 8);
	}

	double float64() const override {
		return val_;
	}

private:
	double val_;
};

class character_value : public ubjson_value {
public:
	vtype type() const override {
		return vtype::character;
	}

	void read(istream& is) override {
		if (is.get() != 'C')
			throw domain_error("Not a character.");
		is >> c_;
	}

	char character() const override {
		return c_;
	}

private:
	char c_;
};

class string_value : public ubjson_value {
public:
	vtype type() const override {
		return vtype::str;
	}

	void read(istream& is) override {
		if (is.get() != 'S')
			throw domain_error("Not a string.");
		ubjson_ptr len = parse(is);
		int64_t l = len->int_value();
		if (l > 0)
			copy_n(istream_iterator<char>(is), l, back_inserter(str_));
	}

	const string& str() const override {
		return str_;
	}

private:
	string str_;
};

class array_value : public ubjson_value {
public:
	vtype type() const override {
		return vtype::array;
	}

	void read(istream& is) override {
		if (is.get() != '[')
			throw domain_error("Not an array.");
		char c = is.peek();
		bool optimized_value = false;
		if (c == '$' || c == '#') {
			if (c == '$') {
				is.get();
				if (is.get() != 'U')
					throw domain_error("Sorry, this parser supports only uint8 optimized format.");
				optimized_value = true;
			}
			if (is.peek() == '#') {
				is.get();
				ubjson_ptr len = parse(is);
				int64_t l = len->int_value();
				if (optimized_value) {
					for (int64_t i = 0; i < l; ++i) {
						uint8_t v;
						is.read(reinterpret_cast<char*>(&v), 1);
						a_.push_back(make_unique<uint8_value>(v));
					}
				}
				else {
					for (int64_t i = 0; i < l; ++i)
						a_.push_back(parse(is));
				}
			}
		}
		else {
			while (is.peek() != ']')
				a_.push_back(parse(is));
			is.get();
		}
	}

	const ubjson_array& ubj_array() const override {
		return a_;
	}

private:
	ubjson_array a_;
};

class object_value : public ubjson_value {
public:
	vtype type() const override {
		return vtype::object;
	}

	void read(istream& is) override {
		if (is.get() != '{')
			throw domain_error("Not an object");

		while (is.peek() != '}') {
			ubjson_ptr len = parse(is);
			int64_t l = len->int_value();
			string key;
			if (l > 0)
				copy_n(istream_iterator<char>(is), l, back_inserter(key));
			ubjson_ptr v = parse(is);
			o_.push_back(make_pair(move(key), move(v)));
		}
		is.get();
	}

	const object& obj() const override {
		return o_;
	}

private:
	object o_;
};

ubjson_ptr ubjson::parse(istream& is) {
	ubjson_ptr p;
	switch (is.peek()) {
	case 'Z':
		p = make_unique<null_value>();
		break;
	case 'N':
		p = make_unique<noop_value>();
		break;
	case 'T':
	case 'F':
		p = make_unique<boolean_value>();
		break;
	case 'i':
		p = make_unique<int8_value>();
		break;
	case 'U':
		p = make_unique<uint8_value>();
		break;
	case 'I':
		p = make_unique<int16_value>();
		break;
	case 'l':
		p = make_unique<int32_value>();
		break;
	case 'd':
		p = make_unique<float32_value>();
		break;
	case 'D':
		p = make_unique<float64_value>();
		break;
	case 'C':
		p = make_unique<character_value>();
		break;
	case 'S':
		p = make_unique<string_value>();
		break;
	case '[':
		p = make_unique<array_value>();
		break;
	case '{':
		p = make_unique<object_value>();
		break;
	default:
		throw domain_error("Option not recognized.");
	}
	p->read(is);
	return p;
}