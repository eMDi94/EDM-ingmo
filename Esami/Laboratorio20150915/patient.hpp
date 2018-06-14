#ifndef PATIENT_HPP
#define PATIENT_HPP

#include <map>
#include <iostream>

namespace patient_ns {
	
	typedef std::map<std::string, std::string> patient;

	patient parse(std::istream& is);

}

#endif // PERSON_HPP