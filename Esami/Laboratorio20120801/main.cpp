#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <iterator>

using namespace std;

using lz78_pair = pair<uint16_t, int8_t>;

void syntax() {
	cerr << "Usage: lz78 [c|d] <input_file> <output_file>\n";
	exit(EXIT_FAILURE);
}

void error(string error_message) {
	cerr << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

inline bool check_extension(const string& filename, string extension) {
	return filename.substr(filename.size() - extension.size()) == extension;
}

class stream_bit_reader {
public:
	explicit stream_bit_reader(istream& is) : is_(is), buffer_(0), count_(0) {}

	template<typename T>
	T get(size_t n) {
		T ret = 0;
		while (n-- > 0)
			ret = (ret << 1) | get_bit();
		return ret;
	}

	bool eos() const {
		if (is_.peek() == EOF)
			return count_ == 0;
		return false;
	}

private:
	istream& is_;
	uint8_t buffer_;
	uint8_t count_;

	uint8_t get_bit() {
		if (count_ == 0) {
			is_.read(reinterpret_cast<char*>(&buffer_), 1);
			count_ = 8;
		}

		return (buffer_ >> (--count_)) & 0x01;
	}
};

class stream_bit_writer {
public:
	explicit stream_bit_writer(ostream& os) : os_(os), buffer_(0), count_(0) {}

	template<typename T>
	void write(T value, size_t n) {
		while (n-- > 0)
			write_bit((value >> n) & 0x01);
	}

	void flush(uint16_t bit = 0) {
		while (count_ != 0)
			write_bit(bit & 0x01);
	}

	~stream_bit_writer() {
		flush();
	}

private:
	ostream& os_;
	uint8_t buffer_;
	uint8_t count_;

	void write_bit(uint8_t bit) {
		buffer_ = (buffer_ << 1) | bit;
		++count_;

		if (count_ == 8) {
			os_.write(reinterpret_cast<const char*>(&buffer_), 1);
			count_ = 0;
		}
	}
};

template<typename T>
size_t bit_count(T val) {
	size_t count;
	for (count = 0; val != 0; val = val >> 1, ++count);
	return count;
}

template<typename T>
T reverse(T val, size_t size = sizeof(T)) {
	T ret = 0;
	for (size_t i = 0; i < size; val = val >> 8, ++i)
		ret = (ret << 8) | (val & 0xFF);
	return ret;
}

template<typename Iter>
void compress(Iter first, Iter last, ostream& os, size_t stream_size) {
	os << "lz78";
	stream_size = reverse(stream_size);
	os.write(reinterpret_cast<const char*>(&stream_size), 4);

	stream_bit_writer bw(os);
	uint16_t last_match_index = 0;
	vector<lz78_pair> dictionary;

	for (; first != last; ++first) {
		int8_t b = *first;
		auto local_p = make_pair(last_match_index, b);
		auto f = find(begin(dictionary), end(dictionary), local_p);
		if (f == end(dictionary)) {
			size_t n_bit = bit_count(dictionary.size());
			if (n_bit > 0)
				bw.write(last_match_index, n_bit);
			bw.write(b, 8);
			if (dictionary.size() < 4095)
				dictionary.emplace_back(last_match_index, b);
			else
				dictionary.clear();
			last_match_index = 0;
		}
		else
			last_match_index = (f - begin(dictionary)) + 1;
	}
}

void lz78_compress(const string& input, const string& output) {
	ifstream is(input, ios::binary);
	if (!is)
		error("Cannot open input file.");
	is.unsetf(ios::skipws);

	if (!check_extension(output, ".lz78"))
		error("Output file must be an .lz78 file.");
	ofstream os(output, ios::binary);
	if (!os)
		error("Cannot open output file.");

	is.seekg(0, ios::end);
	size_t size = is.tellg();
	is.seekg(0, ios::beg);
	compress(istream_iterator<int8_t>(is), istream_iterator<int8_t>(), os, size);
}

template<typename Iter>
void decompress(istream& is, Iter out) {
	string magic = "lz78";
	copy_n(istream_iterator<char>(is), 4, begin(magic));
	if (magic != "lz78")
		error("The file is not an lz78 encoded file.");

	size_t stream_size;
	is.read(reinterpret_cast<char*>(&stream_size), 4);
	stream_size = reverse(stream_size);
	size_t byte_count = 0;

	stream_bit_reader br(is);
	vector<lz78_pair> dictionary;
	while (byte_count < stream_size) {
		uint16_t index = 0;
		size_t n_bit = bit_count(dictionary.size());
		if (n_bit > 0)
			index = br.get<uint16_t>(n_bit);
		int8_t b = br.get<int8_t>(8);
		if (index != 0) {
			uint16_t tmp = index;
			vector<uint16_t> indexes;
			while (tmp != 0) {
				indexes.push_back(tmp);
				tmp = dictionary[tmp - 1].first;
			}

			for (auto r_it = rbegin(indexes); r_it != rend(indexes); ++r_it) {
				*out = dictionary[*r_it - 1].second;
				++out;
				++byte_count;
			}
		}
		*out = b;
		++out;
		++byte_count;

		if (dictionary.size() < 4095)
			dictionary.emplace_back(index, b);
		else
			dictionary.clear();
	}
}

void lz78_decompress(const string& input, const string& output) {
	if (!check_extension(input, ".lz78"))
		error("Input file must be an .lz78 file.");
	ifstream is(input, ios::binary);
	if (!is)
		error("Cannot open input file.");
	is.unsetf(ios::skipws);
	ofstream os(output, ios::binary);
	if (!os)
		error("Cannot open output file.");

	decompress(is, ostream_iterator<int8_t>(os));
}

int main(int argc, char **argv) {
	if (argc != 4)
		syntax();

	string command(argv[1]);
	string input(argv[2]);
	string output(argv[3]);
	if (command == "c")
		lz78_compress(input, output);
	else
		if (command == "d")
			lz78_decompress(input, output);
		else
			error("Option not recognized.");
	cout << "Done!!\n";

	return EXIT_SUCCESS;
}