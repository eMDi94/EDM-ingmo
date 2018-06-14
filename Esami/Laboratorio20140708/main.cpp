#include <vector>
#include <iostream>
#include <iterator>
#include <cstdint>
#include <fstream>
#include <cstdlib>
#include <string>

using namespace std;


/*
 * Error handling
 */

void syntax() {
	cout << "Usage: MOBIdecode <input filename> <output filename>\n";
	exit(EXIT_FAILURE);
}

void error(string&& error_message) {
	cout << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

/*
 * End of Error Handling
 */

/*
 * Utilities
 */

template<typename T>
T flip(T val, size_t count = sizeof(T)) {
	T res = 0;
	for (size_t i = 0; i < count; ++i) {
		res = (res << 8) | (val & 0xFF);
		val = val >> 8;
	}
	return res;
}

inline void skip(istream& is, size_t count) {
	is.seekg(count, ios::cur);
}

inline void check_files(istream& is, ostream& os) {
	if (!is)
		error("Error on opening the input file.");
	if (!os)
		error("Error on opening the output file.");
}

/*
 * End of utilities
 */

/*
 * Write pdb header
 */

size_t out_pdb_header(istream& is) {
	string name(32, 0);
	copy_n(istream_iterator<char>(is), 32, begin(name));
	cout << "PDB Name: " << name << "\n";
	skip(is, 4);

	uint32_t creation_date;
	is.read(reinterpret_cast<char*>(&creation_date), 4);
	creation_date = flip(creation_date);
	cout << "Creation Date (s): " << creation_date << "\n";
	skip(is, 20);

	string type(4, 0), creator(4, 0);
	copy_n(istream_iterator<char>(is), 4, begin(type));
	copy_n(istream_iterator<char>(is), 4, begin(creator));
	if (type != "BOOK" || creator != "MOBI")
		error("Error on type or creator properties.");
	cout << "Type: " << type << "\nCreator: " << creator << "\n";
	skip(is, 8);

	size_t number_of_records = 0;
	is.read(reinterpret_cast<char*>(&number_of_records), 2);
	number_of_records = flip(number_of_records, 2);
	cout << "Records: " << number_of_records << "\n";

	return number_of_records;
}


/*
 * End of writing the pdb header
 */


/*
 * Reading the record info entries
 */

struct record_info_entry {
	size_t record_data_offset;
	uint32_t unique_id;

	record_info_entry(const size_t record_data_offset = 0, const uint32_t unique_id = 0):
					record_data_offset(record_data_offset), unique_id(unique_id) {}
};

vector<record_info_entry> read_record_info_entries(istream& is, const size_t number_of_records) {
	vector<record_info_entry> records;
	records.reserve(number_of_records);

	for (size_t i = 0; i < number_of_records; ++i) {
		size_t offset;
		is.read(reinterpret_cast<char*>(&offset), 4);
		skip(is, 1);
		uint32_t id;
		is.read(reinterpret_cast<char*>(&id), 3);
		record_info_entry record(flip(offset), flip(id, 3));
		records.emplace_back(record);
	}

	return records;
}

template<typename _InIt>
void out_records(_InIt first, _InIt last) {
	for (size_t i = 0; first != last; ++first, ++i) {
		const record_info_entry record = *first;
		cout << i << " - offset: " << record.record_data_offset << " - id: " << record.unique_id << "\n";
	}
}

/*
 * End of record info entries
 */


/*
 * PalmDOC Header
 */

inline string out_compression(const uint16_t compression) {
	switch (compression) {
	case 1:
		return "No Compression";
	case 2:
		return "PalmDOC Compression";
	case 17480:
		return "HUFF/CDIC Compression";
	default:
		error("Compression method not recognized.");
	}
	return "";
}

inline string out_encryption(const uint16_t encryption) {
	switch (encryption) {
	case 0:
		return "No Encryption";
	case 1:
		return "Old Mobipocket Encryption";
	case 2:
		return "Mobipocket Encryption";
	default:
		error("Encryption method not recognized.");
	}
	return "";
}

pair<size_t, size_t> out_palmdoc_header(istream& is, const record_info_entry& record) {
	is.seekg(record.record_data_offset, ios::beg);

	uint16_t compression = 0;
	is.read(reinterpret_cast<char*>(&compression), 2);
	cout << "Compression: " << out_compression(flip(compression)) << "\n";
	skip(is, 2);
	
	size_t text_length = 0, record_count = 0, record_size = 0;
	is.read(reinterpret_cast<char*>(&text_length), 4);
	is.read(reinterpret_cast<char*>(&record_count), 2);
	is.read(reinterpret_cast<char*>(&record_size), 2);
	record_size = flip(record_size, 2);
	if (record_size != 4096)
		error("Record Size not 4096");
	text_length = flip(text_length);
	record_count = flip(record_count, 2);
	cout << "TextLength: " << text_length << "\nRecordCount: " << record_count << "\nRecordSize: " << record_size << "\n";

	uint16_t encryption = 0;
	is.read(reinterpret_cast<char*>(&encryption), 2);
	cout << "Encryption: " << out_encryption(flip(encryption)) << "\n";
	skip(is, 2);

	return { text_length, record_count };
}

/*
 * End of PalmDOC Header
 */

/*
 * Records decoding
 */

inline size_t lz77(istream& is, ostream& os, const uint8_t val, vector<uint8_t>& buffer) {
	uint8_t other = 0;
	is.read(reinterpret_cast<char*>(&other), 1);
	const size_t distance = ((size_t((val & 0x3F) << 8)) + (size_t(other & 0xF8))) >> 3;
	if (distance == 0)
		error("During decompression distance can not be 0.");
	const size_t len = (other & 0x07) + 3;
	const size_t pos = buffer.size();
	for (size_t i = 0; i < len; ++i) {
		const uint8_t buffered_value = buffer[pos - distance + i];
		buffer.emplace_back(buffered_value);
		os << buffered_value;
	}
	return len;
}

inline void reduce_buffer(vector<uint8_t>& buffer) {
	if (buffer.size() > 2047) {
		const size_t surplus = buffer.size() - 2047;
		buffer.erase(begin(buffer), begin(buffer) + surplus);
	}
}

inline size_t decode_record(istream& is, ostream& os, const record_info_entry& record,
							vector<uint8_t>& buffer, const size_t limit = 4096) {
	is.seekg(record.record_data_offset);
	size_t count = 0;
	while (count < limit) {
		uint8_t val = 0;
		is.read(reinterpret_cast<char*>(&val), 1);
		if (val == 0)
			break;
		if (val >= 0x01 && val <= 0x08) {
			copy_n(istream_iterator<uint8_t>(is), val, back_inserter(buffer));
			copy_n(end(buffer) - val, val, ostream_iterator<uint8_t>(os));
			count += val;
		}
		if (val >= 0x09 && val <= 0x7F) {
			os.write(reinterpret_cast<char*>(&val), 1);
			buffer.emplace_back(val);
			++count;
		}
		if (val >= 0x80 && val <= 0xBF)
			count += lz77(is, os, val, buffer);
		if (val >= 0xC0 && val <= 0xFF) {
			os << " " << int8_t(val & 0x7F);
			buffer.emplace_back(' ');
			buffer.emplace_back(val & 0x7F);
			count += 2;
		}

		reduce_buffer(buffer);
	}

	return count;
}

template<typename _InIt>
void decode_records(istream& is, ostream& os, const size_t text_length, const size_t record_count, 
									_InIt first) {
	vector<uint8_t> buffer;
	size_t count = 0;
	for (size_t i = 0; i < record_count - 1; ++i, ++first)
		count += decode_record(is, os, *first, buffer);
	decode_record(is, os, *first, buffer, text_length - count);
}

/*
 * End of records decoding
 */

/*
 * Decode function
 */

inline void write_bom(ostream& os) {
	uint32_t bom = 0x00EFBBBF;
	os.write(reinterpret_cast<const char*>(&bom), 3);
}

void mobi_decode(const string& input_filename, const string& output_filename) {
	ifstream is(input_filename, ios::binary);
	ofstream os(output_filename, ios::binary);
	check_files(is, os);
	is.unsetf(ios::skipws);
	write_bom(os);

	const auto record_count = out_pdb_header(is);
	const auto records = read_record_info_entries(is, record_count);
	out_records(begin(records), end(records));
	const auto p = out_palmdoc_header(is, records[0]);
	decode_records(is, os, p.first, p.second, begin(records) + 1);
}

/*
 * End of decode function
 */


int main(int argc, char **argv) {
	if (argc != 3)
		syntax();
	const string input_filename(argv[1]);
	const string output_filename(argv[2]);

	mobi_decode(input_filename, output_filename);
	cout << "Done!!\n";

	return EXIT_SUCCESS;
}