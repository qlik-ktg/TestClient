#pragma once

#include "cpprest/json.h"

namespace TestClient {

web::json::value JsonCreateBlob(uint64_t size);

} // namespace TestClient
