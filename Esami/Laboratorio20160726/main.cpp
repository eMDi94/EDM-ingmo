#include "bencode.h"
#include "torrent_reader.h"
#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <iomanip>
#include <sstream>


using namespace std;
using namespace torrent;

/*
 * Error handling
 */

void syntax() {
	cout << "Usage: torrent_dump <torrent_input_filename>\n";
	exit(EXIT_FAILURE);
}

void error(string&& error_message) {
	cout << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

bool check_extension(const string& input_filename, string&& extension) {
	return input_filename.substr(input_filename.size() - extension.size(), extension.size()) == extension;
}

/*
 * End of error handling
 */

/*
 * Correctness visitor
 */

class correctness_visitor: public bencode_visitor {
private:

	const bencode_ptr& ptr_;
	bool is_root_, in_info_, info_ok_, from_pieces_, pieces_ok_;

public:

	explicit correctness_visitor(const bencode_ptr& ptr): ptr_(ptr), is_root_(true),
										in_info_(false), info_ok_(false), from_pieces_(false), pieces_ok_(false) {
		ptr_->accept(*this);
	}

	void visit(bencode_dictionary& bencode_dictionary) override {
		if (is_root_)
			is_root_ = false;
		bool is_info = false;
		for (const auto& e : bencode_dictionary) {
			if (e.first == "info") {
				in_info_ = true;
				is_info = true;
				if (e.second->type() != bencode_type::dictionary)
					return;
				info_ok_ = true;
			}
			if (in_info_ && e.first == "pieces" && e.second->type() == bencode_type::string)
				from_pieces_ = true;
			e.second->accept(*this);
			if (is_info && in_info_)
				in_info_ = false;
		}
	}

	void visit(bencode_int& bencode_int) override {}

	void visit(bencode_string& bencode_string) override {
		if (!is_root_) {
			if (from_pieces_ && (bencode_string.element().size() % 20) == 0) {
				pieces_ok_ = true;
				from_pieces_ = false;
			}
		}
	}

	void visit(bencode_list& bencode_list) override {
		if (!is_root_)
			for (const auto& e : bencode_list)
				e->accept(*this);
	}

	operator bool() const {
		return info_ok_ && pieces_ok_;
	}

	bool operator!() const {
		return !bool(*this);
	}

	~correctness_visitor() = default;

};

/*
 * End of correctness visitor
 */


/*
 * Dump visitor
 */

class dump_visitor: public bencode_visitor {
private:

	ostream& os_;
	const bencode_ptr& ptr_;
	bool is_pieces_;
	size_t tabs_;

	
	void write_string(const string& str) const {
		transform(begin(str), end(str), ostream_iterator<char>(os_), [](const char c) -> char {
			if (c < 32 || c > 126)
				return '.';
			return c;
		});
	}

	void add_whitespaces() const {
		for (size_t w = 0; w < tabs_; ++w)
			os_ << " ";
	}

public:
	explicit dump_visitor(ostream& os, const bencode_ptr& ptr): os_(os), ptr_(ptr), is_pieces_(false), tabs_(0) {
		ptr_->accept(*this);
	}

	void visit(bencode_int& bencode_int) override {
		os_ << bencode_int.value() << "\n";
	}

	void visit(bencode_string& bencode_string) override {
		if (!is_pieces_) {
			os_ << "\"";
			write_string(bencode_string.element());
			os_ << "\"\n";
		}
		else {
			os_ << "\n";
			const string& str = bencode_string.element();
			for (size_t i = 0; i < str.size() / 20; ++i) {
				add_whitespaces();
				const string sub = str.substr(i * 20, 20);
				transform(begin(sub), end(sub), ostream_iterator<string>(os_), [](const char c) -> string {
					int32_t r = static_cast<int32_t>(c);
					r = r & 0x000000FF;
					stringstream ss;
					if ((r & 0x000000F0) == 0)
						ss << "0";
					ss << hex << r;
					return ss.str();
				});
				os_ << "\n";
			}
		}
	}

	void visit(bencode_list& bencode_list) override {
		tabs_ += 4;
		os_ << "[\n";
		for (const auto& e : bencode_list) {
			add_whitespaces();
			e->accept(*this);
		}
		tabs_ -= 4;
		add_whitespaces();
		os_ << "]\n";
	}

	void visit(bencode_dictionary& bencode_dictionary) override {
		tabs_ += 4;
		os_ << "{\n";
		for (const auto& e : bencode_dictionary) {
			add_whitespaces();
			write_string(e.first);
			if (e.first == "pieces")
				is_pieces_ = true;
			os_ << " => ";
			tabs_ += e.first.size() + 4;
			e.second->accept(*this);
			tabs_ -= e.first.size() + 4;
		}
		tabs_ -= 4;
		add_whitespaces();
		os_ << "}\n";
	}

	~dump_visitor() = default;
};

/*
 * End of dump visitor
 */


/*
 * Torrent dump
 */

void torrent_dump(const string& input_filename) {
	if (!check_extension(input_filename, ".torrent"))
		error("Input file must be a .torrent file.");
	ifstream is(input_filename, ios::binary);
	is.unsetf(ios::skipws);

	const bencode_ptr ptr = read_bencode(is);

	const correctness_visitor c_visitor(ptr);
	if (!c_visitor)
		error("The torrent file is not formed well.");
	const dump_visitor d_visitor(cout, ptr);
}

/*
 * End of torrent dump
 */


int main(const int argc, char **argv) {
	if (argc != 2)
		syntax();

	const string input_filename(argv[1]);
	torrent_dump(input_filename);

	cout << "Done!!\n";

	return EXIT_SUCCESS;
}