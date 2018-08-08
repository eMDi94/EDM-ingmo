#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <unordered_map>

using namespace std;

void syntax() {
	cerr << "Usage: h261_decode <input_filename>.h261\n";
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
	explicit stream_bit_reader(istream& is) : is_(is), buffer_(0), count_(0), bit_so_far_(0) {}

	template<typename T>
	T get(size_t n) {
		T val = 0;
		bit_so_far_ += n;
		while (n-- > 0)
			val = (val << 1) | read_bit();

		return val;
	}

	bool eos() {
		if (is_.peek() == EOF)
			return count_ == 0;
		return false;
	}

	uint64_t bit_so_far() const {
		return bit_so_far_;
	}

private:
	istream& is_;
	uint8_t buffer_;
	uint8_t count_;
	uint64_t bit_so_far_;

	uint8_t read_bit() {
		if (count_ == 0) {
			is_.read(reinterpret_cast<char*>(&buffer_), 1);
			count_ = 8;
		}

		return (buffer_ >> (--count_)) & 0x01;
	}
};

struct h261_code {
	uint32_t code;
	size_t code_size;

	h261_code(uint32_t code_ = 0, size_t code_size_ = 0) : code(code_), code_size(code_size_) {}

	void add_bit(uint8_t bit) {
		++code_size;
		code = (code << 1) | (bit & 0x01);
	}

	void clear() {
		code_size = 0;
		code = 0;
	}

	bool operator==(const h261_code& rhs) const {
		if (code_size == rhs.code_size)
			return code == rhs.code;
		return false;
	}
};

struct h261_code_hash {
	size_t operator()(const h261_code& code) const {
	hash<uint32_t> h;
	return h(code.code);
	}
};

const unordered_map<h261_code, uint64_t, h261_code_hash> mba_table{
	{{1, 1}, 1},
	{{3, 3}, 2},
	{{2, 3}, 3},
	{{3, 4}, 4},
	{{2, 4}, 5},
	{{3, 5}, 6},
	{{2, 5}, 7},
	{{7, 7}, 8},
	{{6, 7}, 9},
	{{11, 8}, 10},
	{{10, 8}, 11},
	{{9, 8}, 12},
	{{8, 8}, 13},
	{{7, 8}, 14},
	{{6, 8}, 15},
	{{23, 10}, 16},
	{{22, 10}, 17},
	{{21, 10}, 18},
	{{20, 10}, 19},
	{{19, 10}, 20},
	{{18, 10}, 21},
	{{35, 11}, 22},
	{{34, 11}, 23},
	{{33, 11}, 24},
	{{32, 11}, 25},
	{{31, 11}, 26},
	{{30, 11}, 27},
	{{29, 11}, 28},
	{{28, 11}, 29},
	{{27, 11}, 30},
	{{26, 11}, 31},
	{{25, 11}, 32},
	{{24, 11}, 33},
	{{15, 11}, 34},
	{{1, 16}, 0}
};

inline void read_picture(stream_bit_reader& br, ostream& p_os, size_t& current_picture, size_t& current_gn) {
	// Picture
	++current_picture;
	current_gn = 0;
	p_os << current_picture << "\t" << (br.bit_so_far() - 20) << "\t";
	uint8_t tr = br.get<uint8_t>(5);
	p_os << uint32_t(tr) << "\t";
	uint8_t ptype = br.get<uint8_t>(6);
	if ((ptype & 0x04) == 0)
		p_os << "0\n";
	else
		p_os << "1\n";
	uint8_t pei;
	while ((pei = br.get<uint8_t>(1)) == 1)
		br.get<uint8_t>(8);
}

inline uint64_t read_gob_and_first_mb(stream_bit_reader& br, ostream& g_os, ostream& m_os,
						size_t current_picture, size_t current_gn) {
	g_os << current_picture << "\t" << (br.bit_so_far() - 20) << "\t" << current_gn << "\t";
	uint8_t gquant = br.get<uint8_t>(5);
	g_os << uint32_t(gquant) << "\n";
	uint8_t gei;
	while ((gei = br.get<uint8_t>(1)) == 1)
		br.get<uint8_t>(8);

	// Read first MB
	h261_code mba_code;
	uint64_t address;
	while (true) {
		uint8_t bit = br.get<uint8_t>(1);
		mba_code.add_bit(bit);
		auto mba_s = mba_table.find(mba_code);
		if (mba_s != end(mba_table)) {
			address = mba_s->second;
			break;
		}
		if (mba_code.code_size > 16)
			error("MBA address not recognized.");
	}
	if (address != 0)
		m_os << current_picture << "\t" << (br.bit_so_far() - mba_code.code_size) <<
			"\t" << current_gn << "\t" << address << "\t" << mba_code.code_size << "\n";
	return address;
}

inline void read_h216(stream_bit_reader& br, ostream& p_os, ostream& g_os, ostream& m_os) {
	size_t current_picture = 0;
	size_t current_gn = 0;
	auto table_end = end(mba_table);

	h261_code c;
	while (!br.eos()) {
		uint8_t bit = br.get<uint8_t>(1);
		c.add_bit(bit);
		if (c.code_size > 16) {
			uint32_t code_mask = (uint32_t(1) << (c.code_size - 1)) - 1;
			c.code = c.code & code_mask;
			--c.code_size;
		}
		auto s = mba_table.find(c);
		if (s != table_end) {
			uint64_t second = s->second;
			while (second == 0) {
				uint8_t after_4 = br.get<uint8_t>(4);
				if (after_4 == 0) {
					read_picture(br, p_os, current_picture, current_gn);
					second = UINT32_MAX;
				}
				else {
					current_gn = after_4;
					second = read_gob_and_first_mb(br, g_os, m_os, current_picture, current_gn);
				}
			}
			c.clear();
		}
		if (c.code != 0)
			c.clear();
	}
}

void h261_decode(const string& input_filename) {
	if (!check_extension(input_filename, ".h261"))
		error("Input file must be an .h261 file.");
	ifstream is(input_filename, ios::binary);
	if (!is)
		error("Cannot open input file.");
	stream_bit_reader br(is);

	ofstream picture_os("Picture.txt");
	if (!picture_os)
		error("Cannot open output file Picture.txt,");
	ofstream gob_os("GOB.txt");
	if (!gob_os)
		error("Cannot open output file GOB.txt.");
	ofstream mb_os("MB.txt");
	if (!mb_os)
		error("Cannot open output file MB.txt.");

	read_h216(br, picture_os, gob_os, mb_os);
}

int main(int argc, char **argv) {
	if (argc != 2)
		syntax();

	string input(argv[1]);
	h261_decode(input);
	cout << "Done!!\n";

	return EXIT_SUCCESS;
}