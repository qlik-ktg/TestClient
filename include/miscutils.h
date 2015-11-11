#pragma once

#include <string>

namespace TestClient {

/**
 * \brief Compares the content of two files
 *
 * @param fileName1
 * @param fileName2
 * @return 1 = identical, 0 = different
 */
bool CompareFileContent(const std::string& fileName1, const std::string& fileName2, bool useCRC = false);

/**
 * \brief Returns file size
 * @param fileName
 */
size_t GetFileSize(const utility::string_t& fileName);

} // namespace TestClient
