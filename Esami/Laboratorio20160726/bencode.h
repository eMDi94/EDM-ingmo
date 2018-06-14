#ifndef BENCODE_H
#define BENCODE_H

#include <list>
#include <unordered_map>
#include <cstdint>
#include <string>
#include <memory>
#include <iterator>

namespace torrent {
	class bencode_int;

	enum class bencode_type {integer, string, list, dictionary};

	class bencode_visitor;

	class bencode_element {
	public:

		virtual void accept(bencode_visitor& visitor) = 0;

		virtual bencode_type type() const = 0;

		virtual ~bencode_element() = default;

	};

	class bencode_integer;
	class bencode_string;
	class bencode_list;
	class bencode_dictionary;

	typedef std::unique_ptr<bencode_element> bencode_ptr;

	class bencode_visitor {
	public:

		virtual void visit(bencode_int& bencode_int) = 0;

		virtual void visit(bencode_string& bencode_string) = 0;

		virtual void visit(bencode_list& bencode_list) = 0;

		virtual void visit(bencode_dictionary& bencode_dictionary) = 0;

		virtual ~bencode_visitor() = default;

	};

	template<typename T, typename... Args>
	bencode_ptr make_element(Args&&... args) {
		return std::make_unique<T>(std::forward<Args...>(args)...);
	}

	class bencode_int: public bencode_element {
	private:
		int64_t integer_;

	public:
		explicit bencode_int(const int64_t integer): integer_(integer) {}

		bencode_type type() const override {
			return bencode_type::integer;
		}

		void accept(bencode_visitor& visitor) override {
			visitor.visit(*this);
		}

		int64_t value() const {
			return integer_;
		}

	};


	class bencode_string : public bencode_element {
	private:

		std::string string_;

	public:
		explicit bencode_string(std::string&& str): string_(std::move(str)) {}

		bencode_type type() const override {
			return bencode_type::string;
		}

		void accept(bencode_visitor& visitor) override {
			visitor.visit(*this);
		}

		std::string& element() {
			return string_;
		}

		const std::string& element() const {
			return string_;
		}
	};


	class bencode_list : public bencode_element {
	private:

		std::list<bencode_ptr> list_;

	public:
		explicit bencode_list(std::list<bencode_ptr>&& list): list_(std::move(list)) {}

		bencode_type type() const override {
			return bencode_type::list;
		}

		void accept(bencode_visitor& visitor) override {
			visitor.visit(*this);
		}

		std::list<bencode_ptr>& element() {
			return list_;
		}

		const std::list<bencode_ptr>& element() const {
			return list_;
		}

		auto begin() {
			return std::begin(list_);
		}

		auto end() {
			return std::end(list_);
		}

		auto begin() const {
			return std::begin(list_);
		}

		auto end() const {
			return std::end(list_);
		}
	};


	class bencode_dictionary : public bencode_element {
	private:

		std::unordered_map<std::string, bencode_ptr> dictionary_;

	public:
		explicit bencode_dictionary(std::unordered_map<std::string, bencode_ptr>&& dictionary):
												dictionary_(std::move(dictionary)) {}

		bencode_type type() const override {
			return bencode_type::dictionary;
		}

		void accept(bencode_visitor& visitor) override {
			visitor.visit(*this);
		}

		std::unordered_map<std::string, bencode_ptr>& element() {
			return dictionary_;
		}

		const std::unordered_map<std::string, bencode_ptr>& element() const {
			return dictionary_;
		}

		auto begin() {
			return std::begin(dictionary_);
		}

		auto end() {
			return std::end(dictionary_);
		}

		auto begin() const {
			return std::begin(dictionary_);
		}

		auto end() const {
			return std::end(dictionary_);
		}
	};
}

#endif // BENCODE_H