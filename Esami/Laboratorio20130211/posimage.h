#ifndef POSIMAGE_H
#define POSIMAGE_H

#include <cstdint>
#include <memory>
#include <iostream>
#include <vector>

namespace pim {

	enum class pim_type { str, uint, binary, master };

	class pim_element;
	typedef std::unique_ptr<pim_element> pim_ptr;

	class pim_element {
	public:

		explicit pim_element(uint32_t id, size_t id_size, uint64_t size, size_t size_size);

		uint32_t id() const;

		size_t id_size() const;

		size_t size_size() const;

		size_t data_size() const;

		size_t size() const;

		virtual pim_type type() const = 0;

		virtual const std::string& str_value() const;

		virtual uint64_t uint_value() const;

		virtual const std::vector<uint8_t>& binary() const;

		virtual const std::vector<pim_ptr>& elements() const;

	protected:

		uint32_t id_;
		uint64_t size_;
		size_t id_size_, s_size_;

		virtual void read(std::istream& is) = 0;

		friend pim_ptr parse(std::istream& is);
	};

	std::vector<pim_ptr> parse_multi(std::istream& is);
}

#endif // POSIMAGE_H