#ifndef VECTOR_GRAPHICS_H
#define VECTOR_GRAPHICS_H

#include <vector>
#include <memory>
#include <iostream>

namespace vector_graphics {
	
	class element;
	class element_value;

	typedef std::string value;
	typedef std::vector<element> object;

	enum class type {value, object, null};

	class element {
	public:
		/**************
		 * Constructors
		 */
		explicit element();
		explicit element(const std::string& name, value&& val);
		explicit element(const std::string& name, object&& obj);

		/**************
		 * Deleting copy operations
		 */
		element(const element& rhs) = delete;
		element& operator=(const element& rhs) = delete;

		/***************
		 * Move operations
		 */
		element(element&& rhs) noexcept;
		element& operator=(element&& rhs) noexcept;


		/***************
		 * Accessors
		 */
		type type() const;
		const value& value() const;
		const object& object() const;
		const element& operator[](const std::string& key) const;
		bool is_hidden() const;
		bool contains(const std::string& key) const;
		const std::string& element_name() const;

		~element() = default;

	private:
		std::string element_name_;
		std::unique_ptr<element_value> ptr_;
	};

	class element_value {
	protected:
		friend class element;
		virtual type type() const = 0;
		virtual const value& value() const;
		virtual const object& object() const;
		virtual const element& operator[](const std::string& key) const;
		virtual bool is_hidden() const = 0;
		virtual bool contains(const std::string& key) const = 0;

	public:
		virtual ~element_value() = default;
	};

	element parse(std::istream& is);
}

#endif // VECTOR_GRAPHICS_H