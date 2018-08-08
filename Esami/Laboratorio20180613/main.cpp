#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <array>
#include <iomanip>
#include <sstream>

using namespace std;

void syntax() {
	cerr << "Usage: genpdf <input_filename>.txt <output_filename>.pdf\n";
	exit(EXIT_FAILURE);
}

void error(string error_message) {
	cerr << "Error: " << error_message << "\n";
	exit(EXIT_FAILURE);
}

inline bool check_extension(const string& filename, string extension) {
	return filename.substr(filename.size() - extension.size()) == extension;
}

using point = pair<double, double>;

class rectangle {
public:

	double r, g, b;

	rectangle(double x, double y, double width, double height, double r_, double g_, double b_) :
		x_(x), y_(y), width_(width), height_(height), r(r_), g(g_), b(b_) {}

	point coordinates() const {
		double conversion = 2.54 * (1. / 72.);
		return make_pair(x_ / conversion, y_ / conversion);
	}

	point sizes() const {
		double conversion = 2.54 * (1. / 72.);
		return make_pair(width_ / conversion, height_ / conversion);
	}

private:
	double x_, y_, width_, height_;
};

inline vector<rectangle> read_file(const string& input_filename) {
	ifstream is(input_filename);
	if (!is)
		error("Cannot open input file.");
	vector<rectangle> objects;
	while (is.peek() != EOF) {
		double x, y, width, height, r, g, b;
		is >> x >> y >> width >> height >> r >> g >> b >> ws;
		objects.emplace_back(x, y, width, height, r, g, b);
	}
	return objects;
}

inline int64_t write_catalog(ostream& os) {
	int64_t start = os.tellp();
	os << "1 0 obj\n";
	os << "<<\n";
	os << "/Type /Catalog\n";
	os << "/Pages 2 0 R\n";
	os << ">>\n";
	os << "endobj\n";
	return start;
}

inline int64_t write_pages_tree(ostream& os) {
	int64_t start = os.tellp();
	os << "2 0 obj\n";
	os << "<<\n";
	os << "/Type /Pages\n";
	os << "/Count 1\n";
	os << "/Kids [3 0 R]\n";
	os << ">>\n";
	os << "endobj\n";
	return start;
}

inline int64_t write_page(ostream& os) {
	int64_t start = os.tellp();
	os << "3 0 obj\n";
	os << "<<\n";
	os << "/Type /Page\n";
	os << "/Parent 2 0 R\n";
	os << "/Resources << >>\n";
	os << "/MediaBox [ 0 0 595.27559 841.88976 ]\n";
	os << "/Contents 4 0 R\n";
	os << ">>\n";
	os << "endobj\n";
	return start;
}

inline int64_t write_rectangles(ostream& os, const vector<rectangle>& objects) {
	int64_t start = os.tellp();
	os << "4 0 obj\n";
	stringstream ss;
	for (const auto& rect : objects) {
		ss << rect.r << " " << rect.g << " " << rect.b << " rg\n";
		ss << rect.r << " " << rect.g << " " << rect.b << " RG\n";
		auto p = rect.coordinates();
		ss << p.first << " " << p.second << " ";
		auto sizes = rect.sizes();
		ss << sizes.first << " " << sizes.second << " re\n";
		ss << "B\n";
	}
	ss << "endstream";
	auto s = ss.str();
	os << "<< /Length " << s.size() << " >>\n";
	os << "stream\n";
	os << s << "\n";
	os << "endobj\n";
	return start;
}

inline int64_t write_xref_table(ostream& os, const array<int64_t, 4>& pos) {
	int64_t start = os.tellp();
	os << "xref\n";
	os << "0 5\n";
	os << "0000000000 65535 f\n";
	for (const auto& p : pos) {
		os << setw(10) << setfill('0') << p;
		os << " ";
		os << setw(5) << setfill('0') << "0";
		os << " n\n";
	}
	return start;
}

inline void write_trailer(ostream& os, int64_t x_ref_pos) {
	os << "trailer\n";
	os << "<<\n";
	os << "/Size 5\n";
	os << "/Root 1 0 R\n";
	os << ">>\nstartxref\n";
	os << x_ref_pos << "\n";
	os << "%%EOF";
}

inline void write_pdf(ostream& os, const vector<rectangle>& objects) {
	// write pdf header
	os << "%PDF-1.3\n";
	array<int64_t, 4> pos;
	pos[0] = write_catalog(os);
	pos[1] = write_pages_tree(os);
	pos[2] = write_page(os);
	pos[3] = write_rectangles(os, objects);
	int64_t x_ref_pos = write_xref_table(os, pos);
	write_trailer(os, x_ref_pos);
}

void genpdf(const string& input_filename, const string& output_filename) {
	if (!check_extension(input_filename, ".txt"))
		error("Input file must be a .txt file.");
	if (!check_extension(output_filename, ".pdf"))
		error("Output file must be a .pdf file.");
	const auto objects = read_file(input_filename);
	ofstream os(output_filename, ios::binary);
	if (!os)
		error("Cannot open output file.");
	write_pdf(os, objects);
}

int main(int argc, char **argv) {
	if (argc != 3)
		syntax();

	string input(argv[1]);
	string output(argv[2]);
	genpdf(input, output);
	cout << "Done!!!\n";

	return EXIT_SUCCESS;
}