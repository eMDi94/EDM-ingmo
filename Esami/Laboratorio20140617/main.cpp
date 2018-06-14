#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>
#include <cstdlib>
#include <unordered_map>
#include <stdexcept>
#include <algorithm>
#include <iterator>

using namespace std;

void syntax() {
	cerr << "Usage: lz78encode <max bits> <input filename> <output filename>\n";
	exit(EXIT_FAILURE);
}

void error(string error_message) {
	cerr << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

void pretty_byte(uint32_t byte, size_t n) {
	while (n-- > 0)
		cout << ((byte >> n) & 0x01);
	cout << "\t";
}

class stream_bit_writer {
public:
	explicit stream_bit_writer(ostream& os) : os_(os), buffer_(0), count_(8) {}

	template<typename T>
	void write_value(T value, size_t n) {
		while (n-- > 0)
			write_bit((value >> n) & 0x01);
	}

	void flush(uint8_t value = 0) {
		while (count_ != 8)
			write_bit(value);
	}

	~stream_bit_writer() {
		flush();
	}

private:
	ostream& os_;
	uint8_t buffer_;
	size_t count_;

	void write_bit(uint8_t bit) {
		bit = bit & 0x01;
		buffer_ = buffer_ | (bit << --count_);
		
		if (count_ == 0) {
			os_.write(reinterpret_cast<const char*>(&buffer_), 1);
			buffer_ = 0;
			count_ = 8;
		}
	}
};

inline void init_dictionary(unordered_map<string, size_t>& dictionary) {
	dictionary.clear();
	dictionary[""] = 0;
}

inline size_t max_used_bits(size_t dict_size) {
	size_t i = 0;
	for (i = 0; i < 32 && dict_size != 0; ++i, dict_size = dict_size >> 1);
	if (i == 32)
		error("Max bits too long.");
	return i;
}

inline void lz78_encode(istream& is, stream_bit_writer& bw, size_t max_bits) {
	is.unsetf(ios::skipws);
	unordered_map<string, size_t> dictionary;
	init_dictionary(dictionary);
	size_t max_size = size_t(1) << max_bits;
	string buffer;
	size_t current_index = 0;
	size_t last_inserted_index = 0;
	char c;
	while (true) {
		is >> c;
		buffer += c;
		if (is.peek() == EOF)
			break;
		auto it = dictionary.find(buffer);
		if (it != end(dictionary))
			current_index = it->second;
		else {
			size_t n_bits = max_used_bits(last_inserted_index);
			bw.write_value(current_index, n_bits);
			bw.write_value(c, 8);
			if (dictionary.size() == max_size) {
				last_inserted_index = 0;
				init_dictionary(dictionary);
			}
			else
				dictionary[buffer] = ++last_inserted_index;
			buffer.clear();
			current_index = 0;
		}
	}
	if (!buffer.empty()) {
		size_t n_bits = max_used_bits(last_inserted_index);
		bw.write_value(current_index, n_bits);
		bw.write_value(c, 8);
	}
	is.setf(ios::skipws);
}

void encode(size_t max_bits, const string& input_filename, const string& output_filename) {
	if (max_bits < 1 || max_bits > 31)
		throw invalid_argument("");
	ifstream is(input_filename, ios::binary);
	if (!is)
		error("Cannot open the input file.");
	ofstream os(output_filename, ios::binary);
	if (!os)
		error("Cannot open the output file.");
	os << "LZ78";
	stream_bit_writer bw(os);
	bw.write_value(max_bits, 5);
	lz78_encode(is, bw, max_bits);
}


int main(int argc, char **argv) {
	if (argc != 4)
		syntax();
	
	try {
		size_t max_bits = size_t(stoul(string(argv[1])));

		string input(argv[2]);
		string output(argv[3]);

		encode(max_bits, input, output);
		cout << "Done\n";

		return EXIT_SUCCESS;
	}
	catch (invalid_argument&) {
		error("First parameter must be a number between 1 and 31.");
	}
}