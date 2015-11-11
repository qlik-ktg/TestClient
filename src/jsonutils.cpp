#include "jsonutils.h"

namespace TestClient {

web::json::value JsonCreateBlob(uint64_t size) {
    web::json::value blob = web::json::value::object(true);

    web::json::value data = web::json::value::object(true);
    data[U("type")] = web::json::value::string(U("Blob"));
    web::json::value attributes = web::json::value::object(true);
    attributes[U("contentType")] = web::json::value::string(U("application/octet-stream"));
    attributes[U("contentLength")] = web::json::value::number(size);
    data[U("attributes")] = attributes;


    blob[U("data")] = data;

    return blob;
}

} // namespace TestClient
