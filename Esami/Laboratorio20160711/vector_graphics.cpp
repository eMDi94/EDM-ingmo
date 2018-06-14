#include "vector_graphics.h"
#include <stdexcept>
#include <string>
#include <iterator>

using namespace std;
using namespace vector_graphics;


////////////////////////////////////////////
/* element_value dummy implementation*/
///////////////////////////////////////////

const value& element_value::value() const {
	throw logic_error("Not implemented.");
}

const object& element_value::object() const {
	throw logic_error("Not implemented.");
}

const element& element_value::operator[](const std::string& key) const {
	throw logic_error("Not implemented.");
}


////////////////////////////////////////////////
/* implementation of different values */
///////////////////////////////////////////////

class vector_null;

template<typename T, type Tag>
class value_: public element_value {
protected:
	explicit value_(T&& val): val_(move(val)) {}

	vector_graphics::type type() const override {
		return Tag;
	}

	const T val_;

	virtual ~value_() = default;

	const static element e_null;
};

template<typename T, type Tag>
const element value_<T, Tag>::e_null = element();

class vector_value: public value_<value, type::value> {
protected:
	bool is_hidden() const override {
		return false;
	}

	bool contains(const std::string& key) const override {
		return false;
	}

	const vector_graphics::value& value() const override {
		return val_;
	}

public:
	explicit vector_value(vector_graphics::value&& val): value_(move(val)) {}
};


class vector_null: public value_<nullptr_t, type::null> {
protected:
	bool is_hidden() const override {
		return false;
	}

	bool contains(const std::string& key) const override {
		return false;
	}

public:
	explicit vector_null() : value_(nullptr) {};
};


class vector_object: public value_<object, type::object> {
protected:
	bool is_hidden() const override {
		for (const auto& e : val_)
			if (e.element_name() == "hidden" && e.type() == type::value)
				return e.value() == "true";
		return false;
	}

	size_t _contains(const std::string& key) const {
		for (size_t i = 0; i < val_.size(); ++i)
			if (val_.at(i).element_name() == key)
				return i;
		return val_.size();
	}

	const vector_graphics::object& object() const override {
		return val_;
	}

	bool contains(const std::string& key) const override {
		return _contains(key) != val_.size();
	}

	const element& operator[](const string &key) const override {
		const size_t index = _contains(key);
		return index == val_.size() ? e_null : val_.at(index);
	}

public:
	explicit vector_object(vector_graphics::object&& obj): value_(move(obj)) {}
};


/////////////////////////////////////////////////////
/* vector_graphics element*/
/////////////////////////////////////////////////////

element::element(): element_name_(), ptr_(make_unique<vector_null>()) {}

element::element(const std::string& name, vector_graphics::object&& obj):
											element_name_(name),
											ptr_(make_unique<vector_object>(forward<vector_graphics::object>(obj))) {}

element::element(const std::string& name, vector_graphics::value&& val):
											element_name_(name),
											ptr_(make_unique<vector_value>(forward<vector_graphics::value>(val))){}

element::element(element&& rhs) noexcept: element_name_(move(rhs.element_name_)), ptr_(move(rhs.ptr_)) {}

element& element::operator=(element&& rhs) noexcept {
	swap(element_name_, rhs.element_name_);
	swap(ptr_, rhs.ptr_);
	return *this;
}

type element::type() const {
	return ptr_->type();
}

const value& element::value() const {
	return ptr_->value();
}

const object& element::object() const {
	return ptr_->object();
}

bool element::is_hidden() const {
	return ptr_->is_hidden();
}

const string& element::element_name() const {
	return element_name_;
}

bool element::contains(const string& key) const {
	return ptr_->contains(key);
}

const element& element::operator[](const string& key) const {
	return (*ptr_)[key];
}

////////////////////////////////////////////////
/*Parsing the file*/
////////////////////////////////////////////////

value read_value(istream& is) {
	is.unsetf(ios::skipws);
	string val;
	//Remove the first "
	is.get();
	char c1 = 0, c2 = 0;
	while (true) {
		is >> c2;
		if (c1 == '"') {
			if (c2 == '"') {
				val.push_back(c2);
				c2 = 0;
			}
			else
				break;
		}
		else {
			if (c2 != '"')
				val.push_back(c2);
		}
		c1 = c2;
		if (!is)
			throw logic_error("Never ending value.");
	}

	is.setf(ios::skipws);
	return val;
}

object read_object(istream& is) {
	object obj;
	element e;
	while ((e = parse(is)).type() != type::null) {
		obj.push_back(move(e));
	}
	return obj;
}


element vector_graphics::parse(istream& is) {
	string id;
	is >> id >> ws;
	if (!is)
		throw logic_error("File ended with an id.");
	const char c = is.peek();
	switch (c) {
	case 'o': {
		string obj_;
		is >> obj_;
		object obj = read_object(is);
		return element(id, move(obj));
	}
	case '"': {
		value val = read_value(is);
		return element(id, move(val));
	}
	case 'e': {
		string end;
		is >> end;
		if (end != "end")
			throw logic_error("Id obj not followed by an end.");
		return element();
	}
	default:
		throw logic_error("Option not recognized.");
	}
}