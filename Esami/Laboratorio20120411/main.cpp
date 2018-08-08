#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>
#include <cstdlib>

using namespace std;

void print_byte(uint8_t b) {
	uint8_t mask = 0x80;
	for (size_t i = 0; i < 8; ++i, mask = mask >> 1)
		if ((b & mask) != 0)
			cout << "1";
		else
			cout << "0";
	cout << "\n";
}

class stream_bit_writer {
public:
	explicit stream_bit_writer(ostream& os) : os_(os), buffer_(0), count_(8) {}

	template<typename T>
	void write(T val, size_t n) {
		while (n-- > 0)
			write_bit(val >> n);
	}

	void flush(uint8_t bit = 1) {
		while (count_ != 8)
			write_bit(bit);
	}

	~stream_bit_writer() {
		flush();
	}

private:
	ostream& os_;
	uint8_t buffer_;
	uint8_t count_;

	void write_bit(uint8_t bit) {
		buffer_ = buffer_ | ((bit & 0x01) << (--count_));

		if (count_ == 0) {
			print_byte(buffer_);
			os_.write(reinterpret_cast<const char*>(&buffer_), 1);
			count_ = 8;
			buffer_ = 0;
		}
	}
};

void syntax() {
	cerr << "Usage: coduna <input_filename>\n";
	exit(EXIT_FAILURE);
}

void error(string error_message) {
	cerr << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

template<typename T>
T flip(T val, size_t size = sizeof(T)) {
	T ret = 0;
	for (size_t i = 0; i < size; ++i, val = val >> 8)
		ret = (ret << 8) | (val & 0xFF);
	return ret;
}

void unary_coding(const string& input_filename) {
	ifstream is(input_filename, ios::binary);
	if (!is)
		error("Cannot open input file.");
	is.unsetf(ios::skipws);
	ofstream os(input_filename + ".una", ios::binary);
	if (!os)
		error("Cannot open output file.");
	stream_bit_writer bw(os);

	os << "UNARIO";
	is.seekg(0, ios::end);
	size_t len = is.tellg();
	len = flip(len);
	os.write(reinterpret_cast<const char*>(&len), 4);
	is.seekg(0, ios::beg);
	
	while (is.peek() != EOF) {
		uint8_t value;
		is.read(reinterpret_cast<char*>(&value), 1);
		for (uint8_t i = 0; i < value; ++i)
			bw.write(1, 1);
		bw.write(0, 1);
	}
}

int main(int argc, char **argv) {
	if (argc != 2)
		syntax();

	string input(argv[1]);
	unary_coding(input);
	cout << "Done!!\n";

	return EXIT_SUCCESS;
}