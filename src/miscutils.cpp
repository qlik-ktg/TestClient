#include "miscutils.h"

#include <iostream>
#include <string>
#include <stdio.h>
#include <chrono>

#include "boost/iostreams/device/mapped_file.hpp"
#include "boost/crc.hpp"

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

using namespace std;

namespace TestClient {

bool CompareFileContent(const std::string& fileName1, const std::string& fileName2, bool useCRC /*= false*/) {
    namespace io = boost::iostreams;

    io::mapped_file_source f1(fileName1.c_str());
    io::mapped_file_source f2(fileName2.c_str());

    if (!useCRC) {
        if (f1.size() == f2.size()
            && std::equal(f1.data(), f1.data() + f1.size(), f2.data())) {
            return true; // identical
        }
        else
            return false; // different
    }
    else {
        boost::crc_optimal<64, 0x04C11DB7, 0, 0, false, false> crc;
        crc.process_block(f1.data(), f1.data() + f1.size());
        auto checksum1 = crc.checksum();

        crc.reset();
        crc.process_block(f2.data(), f2.data() + f2.size());
        auto checksum2 = crc.checksum();

        return (checksum1 == checksum2);
    }
}

size_t GetFileSize(const utility::string_t& fileName) {
    std::string convFileName(fileName.begin(), fileName.end());

    std::ifstream is;
    is.open(convFileName.c_str(), std::ios::binary);
    is.seekg(0, std::ios::end);
    auto length = is.tellg();

    is.close();

    return length;
}

} // namespace TestClient
