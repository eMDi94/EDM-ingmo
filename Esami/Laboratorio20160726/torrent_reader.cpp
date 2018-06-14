#include "torrent_reader.h"
#include <stdexcept>
#include <algorithm>

using namespace std;
using namespace torrent;


int64_t read_integer(istream& is) {
	// Skip the i
	is.get();

	// Read the value
	int64_t value = 0;
	is >> value;
	if (is.get() != 'e')
		throw invalid_argument("Integer not terminated by an e.");

	return value;
}

string read_string(istream& is) {
	size_t string_size = 0;
	is >> string_size;

	if (is.get() != ':')
		throw invalid_argument("String not separated by a :.");

	string b_str(string_size, '0');
	if (string_size > 0)
		copy_n(istream_iterator<char>(is), string_size, begin(b_str));

	return b_str;
}

list<bencode_ptr> read_list(istream& is) {
	// Skip the l
	is.get();

	list<bencode_ptr> list;

	while (is.peek() != 'e') {
		list.emplace_back(read_bencode(is));
		if (!is)
			throw invalid_argument("Error during reading the list.");
	}

	is.get();

	return list;
}

unordered_map<string, bencode_ptr> read_dictionary(istream& is) {
	// Skip the d
	is.get();

	unordered_map<string, bencode_ptr> dictionary;

	while (is.peek() != 'e') {
		if (!isdigit(is.peek()))
			throw invalid_argument("Dictionary without a key.");
		size_t d_size = dictionary.size();
		string key = read_string(is);
		bencode_ptr b_el = read_bencode(is);
		dictionary.emplace(make_pair(move(key), move(b_el)));
	}

	is.get();

	return dictionary;
}


bencode_ptr torrent::read_bencode(istream & is) {
	const char p = is.peek();

	if (isdigit(p))
		return make_element<bencode_string>(read_string(is));
	else
		switch (p) {
		case 'i':
			return make_element<bencode_int>(read_integer(is));
		case 'l':
			return make_element<bencode_list>(read_list(is));
		case 'd':
			return make_element<bencode_dictionary>(read_dictionary(is));
		default:
			throw invalid_argument("Format not recognized.");
		}
}
