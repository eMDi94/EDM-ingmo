#ifndef DICOM_HPP
#define DICOM_HPP

#include <iostream>
#include <string>
#include <array>

namespace dicom {
	
	class dicom_writer {
	public:
		explicit dicom_writer(std::ostream& os);

		dicom_writer(const dicom_writer& rhs) = default;
		dicom_writer& operator=(const dicom_writer& rhs) = delete;

		dicom_writer(dicom_writer&& rhs) = default;
		dicom_writer& operator=(dicom_writer&& rhs) = delete;

		bool write_string(uint16_t group, uint16_t element, std::string& vr, const std::string& value) const;
		bool write_patient_name(const std::string& first_name, const std::string& last_name) const;
		bool write_value(uint16_t group, uint16_t element, const std::string& vr, int64_t value) const;
		bool write_group2_size() const;
		
		template<typename T>
		bool write_byte_stream(uint16_t group, uint16_t element, T* data, size_t size) const {
			write(group | uint32_t(element) << 16, 4);
			os_ << "OB";
			size_t l_size = size;
			if (l_size % 2 != 0)
				++l_size;
			write(uint16_t(0) | int64_t(l_size) << 16, 6);
			os_.write(reinterpret_cast<const char*>(data), size);
			if (size != l_size)
				os_ << uint8_t(0);

			if (!os_)
				return false;
			return true;
		}

		operator bool() const;

		~dicom_writer() = default;
	private:
		std::ostream& os_;

		void write(int64_t value, size_t size) const;

		static const std::array<std::string, 7> tags;
	};

}

#endif // DICOM_HPP