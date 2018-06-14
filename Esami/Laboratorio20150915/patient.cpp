#include "patient.hpp"
#include <string>
#include <iomanip>

using namespace std;
using namespace patient_ns;


patient patient_ns::parse(istream& is) {
	patient p;
	while (is) {
		string field;
		getline(is, field, ':');
		is >> ws;
		string value;
		is >> value >> ws;
		p.insert(make_pair(move(field), move(value)));
	}
	return p;
}
