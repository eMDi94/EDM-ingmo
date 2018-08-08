#include "json.h"
#include <stdexcept>

using namespace std;
using namespace Json;


/*
 * Json Value dummy implementation
 */

int64_t json_value::int_value() {
	throw logic_error("Not implemented.");
}

int64_t json_value::int_value() const {
	throw logic_error("Not implemented.");
}

string& json_value::string_value() {
	throw logic_error("Not implemented.");
}

const string& json_value::string_value() const {
	throw logic_error("Not implemented.");
}

Json::array& json_value::array_items() {
	throw logic_error("Not implemented.");
}

const Json::array& json_value::array_items() const {
	throw logic_error("Not implemented.");
}

object& json_value::object_items() {
	throw logic_error("Not implemented.");
}

const object& json_value::object_items() const {
	throw logic_error("Not implemented.");
}

json& json_value::operator[](const size_t index) {
	throw logic_error("Not implemented.");
}

const json& json_value::operator[](const size_t index) const {
	throw logic_error("Not implemented.");
}

json& json_value::operator[](const string& key) {
	throw logic_error("Not implemented.");
}

const json& json_value::operator[](const string& key) const {
	throw logic_error("Not implemented.");
}

/*
 * End of dummy implementation
 */

/*
 * Json Value classes
 */

template<typename T, json_type Type>
class value: public json_value {
protected:

	static json json_null_;

	explicit value(const T& val): val_(val) {}
	explicit value(T&& val): val_(move(val)) {}

	json_type type() const override {
		return Type;
	}

	T val_;
};

template<typename T, json_type Type>
json value<T, Type>::json_null_ = json();

class json_null: public value<std::nullptr_t, json_type::null> {
public:
	explicit json_null(): value(nullptr) {}

	static const json_null static_null;
};

const json_null static_null = json_null();

class json_int: public value<int64_t, json_type::integer> {
protected:
	int64_t int_value() override {
		return val_;
	}

	int64_t int_value() const override {
		return val_;
	}

public:
	explicit json_int(const int64_t val): value(val) {}
};

class json_string: public value<string, json_type::string> {
protected:
	string& string_value() override {
		return val_;
	}

	const string& string_value() const override {
		return val_;
	}

public:
	explicit json_string(const string& str): value(str) {}
	explicit json_string(string&& str): value(move(str)) {}
};

class json_array: public value<Json::array, json_type::array> {
protected:
	json& operator[](const size_t index) override {
		if (index >= val_.size())
			return json_null_;
		auto b = begin(val_);
		advance(b, index);
		return *(b);
	}

	const json& operator[](const size_t index) const override {
		if (index >= val_.size())
			return json_null_;
		auto b = begin(val_);
		advance(b, index);
		return *b;
	}

	Json::array& array_items() override {
		return val_;
	}

	const Json::array& array_items() const override {
		return val_;
	}

public:
	explicit json_array(const Json::array& ar): value(ar) {}
	explicit json_array(Json::array&& ar): value(move(ar)) {}
};

class json_object: public value<object, json_type::object> {
protected:
	json& operator[](const string& key) override {
		auto it = val_.find(key);
		if (it == end(val_))
			return json_null_;
		return it->second;
	}

	const json& operator[](const string& key) const override {
		auto it = val_.find(key);
		if (it == end(val_))
			return json_null_;
		return it->second;
	}

	object& object_items() override {
		return val_;
	}

	const object& object_items() const override {
		return val_;
	}

public:
	explicit json_object(const object& obj): value(obj) {}
	explicit json_object(object&& obj): value(move(obj)) {}
};

/*
 * End of json value classes
 */


/*
 * Json class implementation
 */

json::json(): j_ptr_(make_shared<json_null>()) {}

json::json(std::nullptr_t): j_ptr_(make_shared<json_null>()) {}

json::json(int64_t value): j_ptr_(make_shared<json_int>(value)) {}

json::json(const string& str): j_ptr_(make_shared<json_string>(str)) {}

json::json(string&& str): j_ptr_(make_shared<json_string>(forward<string>(str))) {}

json::json(const char *str): j_ptr_(make_shared<json_string>(str)) {}

json::json(const Json::array& ar): j_ptr_(make_shared<json_array>(ar)) {}

json::json(Json::array&& ar): j_ptr_(make_shared<json_array>(forward<array>(ar))) {}

json::json(const object& obj): j_ptr_(make_shared<json_object>(obj)) {}

json::json(object&& obj): j_ptr_(make_shared<json_object>(forward<object>(obj))) {}

json::json(json&& rhs): j_ptr_(move(rhs.j_ptr_)) {}

json& json::operator=(json&& rhs) {
	swap(j_ptr_, rhs.j_ptr_);
	return *this;
}

int64_t json::int_value() {
	return j_ptr_->int_value();
}

int64_t json::int_value() const {
	return j_ptr_->int_value();
}

string& json::string_value() {
	return j_ptr_->string_value();
}

const string& json::string_value() const {
	return j_ptr_->string_value();
}

Json::array& json::array_items() {
	return j_ptr_->array_items();
}

const Json::array& json::array_items() const {
	return j_ptr_->array_items();
}

object& json::object_items() {
	return j_ptr_->object_items();
}

const object& json::object_items() const {
	return j_ptr_->object_items();
}

json& json::operator[](const size_t index) {
	return (*j_ptr_)[index];
}

const json& json::operator[](const size_t index) const {
	return (*j_ptr_)[index];
}

json& json::operator[](const string& key) {
	return (*j_ptr_)[key];
}

const json& json::operator[](const string& key) const {
	return (*j_ptr_)[key];
}

json_type json::type() const {
	return j_ptr_->type();
}

/*
 * End of json class implementation
 */



 /*
 * Parsing
 */

 
std::string parse_string(istream& is) {
	char c;
	is >> c;
	if (c != '"' || !is)
		throw logic_error("Error during reading the string.");
	string str;
	getline(is, str, '"');
	return str;
}

int64_t parse_int(istream& is) {
	int64_t value;
	is >> value;
	return value;
}

Json::array parse_array(istream& is) {
	Json::array ar;
	char array_beg;
	is >> array_beg;
	if (!is || array_beg != '[')
		throw logic_error("json array error: not starting with [");

	is >> ws;
	char array_end = ',';
	while (is && array_end == ',' && array_end != ']') {
		json item = parse(is);
		ar.push_back(move(item));
		is >> array_end;
	}

	if (!is && array_end != ']')
		throw logic_error("json array error: not ending with ]");

	return ar;
}

object parse_object(istream& is) {
	object obj;
	char obj_beg;
	is >> obj_beg;
	if (!is || obj_beg != '{')
		throw logic_error("json object error: not starting with {");

	is >> ws;
	char obj_end = ',';
	while (is && obj_end == ',' && obj_end != '}') {
		json key = parse(is);
		if (key.type() != json_type::string)
			throw logic_error("json object error: key is not a string.");
		string str_key = key.string_value();
		char colon;
		is >> colon;
		if (colon != ':')
			throw logic_error("json object error: key and value not separated by a colon.");
		json value = parse(is);
		obj.insert(make_pair(move(key.string_value()), move(value)));
		is >> obj_end;
	}

	if (!is && obj_end != '}')
		throw logic_error("json object error: object not ending with a }");

	return obj;
 }

 json Json::parse(istream& is) {
	 is >> ws;
	 const char peek = is.peek();
	 if (isdigit(peek)) {
		 int64_t value = parse_int(is);
		 return json(value);
	 }
	 switch (peek) {
	 case '"': {
		 string str = parse_string(is);
		 return json(move(str));
	 }
	 case '[': {
		 Json::array ar = parse_array(is);
		 return json(move(ar));
	 }
	 case '{': {
		 object obj = parse_object(is);
		 return json(move(obj));
	 }
	 default:
		throw logic_error("Not a valid json file.");
	 }
 }
 

 /*
 * End of parsing
 */