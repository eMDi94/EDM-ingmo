#ifndef TORRENT_READER_H
#define TORRENT_READER_H

#include "bencode.h"
#include <iostream>

namespace torrent {

	bencode_ptr read_bencode(std::istream& is);

}

#endif // TORRENT_READER_H