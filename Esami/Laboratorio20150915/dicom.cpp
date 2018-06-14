#include "dicom.hpp"
#include <algorithm>
#include <iterator>
#include <sstream>
#include <vector>

using namespace std;
using namespace dicom;


const array<string, 7> dicom_writer::tags = { "US", "UL", "CS", "LO", "PN", "UI", "OB" };

dicom_writer::dicom_writer(std::ostream& os): os_(os) {
	fill_n(ostream_iterator<uint8_t>(os), 128, 0);
	os_ << "DICM";
}

void dicom_writer::write(int64_t value, size_t size) const {
	os_.write(reinterpret_cast<const char*>(&value), size);
}

bool dicom_writer::write_string(uint16_t group, uint16_t element, string& vr, const string& value) const {
	write(group | uint32_t(element) << 16, 4);
	transform(begin(vr), end(vr), begin(vr), toupper);
	const auto it = find(begin(tags), end(tags), vr);
	if (it == end(tags))
		return false;
	const size_t index = it - begin(tags);
	if (index == 0 || index == 1 || index == 7)
		return false;
	os_ << vr;
	vector<char> buffer(begin(value), end(value));
	if (value.size() % 2 != 0) {
		if (vr == "CS" || vr == "LO")
			buffer.emplace_back(32);
		else
			buffer.emplace_back(0);
		write(value.size() + 1, 2);
	}
	else
		write(value.size(), 2);
	move(begin(buffer), end(buffer), ostream_iterator<char>(os_));
	if (!os_)
		return false;
	return true;
}


bool dicom_writer::write_patient_name(const std::string& first_name, const std::string& last_name) const {
	string person_name = first_name;
	person_name += "^";
	person_name += last_name;
	string tag = "PN";
	return write_string(0x0010, 0x0010, tag, person_name);
}

bool dicom_writer::write_value(uint16_t group, uint16_t element, const std::string& vr, int64_t value) const {
	write(group | uint32_t(element) << 16, 4);
	if (vr != "US" && vr != "UL")
		return false;
	if (vr == "US") {
		os_ << vr;
		write(2, 2);
		write(value, 2);
	}
	if (vr == "UL") {
		os_ << vr;
		write(4, 2);
		write(value, 4);
	}
	if (!os_)
		return false;
	return true;
}

bool dicom_writer::write_group2_size() const {
	int64_t size = os_.tellp();
	size -= 144;
	os_.seekp(size_t(140), ios::beg);
	write(size, 4);
	os_.seekp(0, ios::end);
	return true;
}


dicom_writer::operator bool() const {
	return os_.good();
}
