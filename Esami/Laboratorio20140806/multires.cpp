#include "multires.h"

using namespace std;
using namespace core;
using namespace multires;


const core::mat<size_t> _multires_base::adam7_mat = core::mat<size_t>(8, 8,
	std::array<size_t, 64>{
	1, 6, 4, 6, 2, 6, 4, 6,
		7, 7, 7, 7, 7, 7, 7, 7,
		5, 6, 5, 6, 5, 6, 5, 6,
		7, 7, 7, 7, 7, 7, 7, 7,
		3, 6, 4, 6, 3, 6, 4, 6,
		7, 7, 7, 7, 7, 7, 7, 7,
		5, 6, 5, 6, 5, 6, 5, 6,
		7, 7, 7, 7, 7, 7, 7, 7}.data());


multires_encoder::multires_encoder(const core::mat<uint8_t>& image) {
	for (size_t r = 0; r < image.height(); ++r) {
		for (size_t c = 0; c < image.width(); ++c) {
			const size_t level = adam7_mat(r % 8, c % 8);
			levels_[level - 1].emplace_back(image(r, c));
		}
	}
}

vector<uint8_t>& multires_encoder::get(const size_t index) const {
	if (index >= levels_.size())
		throw out_of_range("Index out of level range.");
	return const_cast<vector<uint8_t>&>(levels_[index]);
}

vector<uint8_t>& multires_encoder::operator[](const size_t index) {
	return get(index);
}

const vector<uint8_t>& multires_encoder::operator[](const size_t index) const {
	return get(index);
}


/****************************************************************************************/