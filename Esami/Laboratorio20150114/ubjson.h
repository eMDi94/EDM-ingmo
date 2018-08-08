#ifndef UBJSON_H
#define UBJSON_H

#include <iostream>
#include <cstdint>
#include <vector>
#include <memory>
#include <utility>

namespace ubjson {

	class ubjson_value;
	typedef std::unique_ptr<ubjson_value> ubjson_ptr;
	typedef std::vector<ubjson_ptr> ubjson_array;
	typedef std::pair<std::string, ubjson_ptr> object_p;
	typedef std::vector<object_p> object;

	enum class vtype {
		null, noop, boolean, int8, uint8, int16, int32, float32, float64, character, str, array, object
	};

	class ubjson_value {
	public:

		virtual vtype type() const = 0;
		virtual void read(std::istream& is) = 0;
		virtual bool boolean() const;
		virtual int8_t int8() const;
		virtual uint8_t uint8() const;
		virtual int16_t int16() const;
		virtual int32_t int32() const;
		virtual float float32() const;
		virtual double float64() const;
		virtual char character() const;
		virtual const std::string& str() const;
		virtual const ubjson_array& ubj_array() const;
		virtual const object& obj() const;

		int64_t int_value() const;
	};

	ubjson_ptr parse(std::istream& is);

}

#endif // UBJSON_H