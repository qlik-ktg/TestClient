#include "contentservice.h"
#include "jsonutils.h"

#include "cpprest/http_client.h"
#include "cpprest/json.h"
#include "cpprest/filestream.h"
#include "cpprest/containerstream.h"
#include "cpprest/streams.h"
#include "cpprest/rawptrstream.h"

#include <chrono>

using namespace std;

using namespace ::pplx;
using namespace utility;
using namespace Concurrency::streams;

using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace web::json;


namespace TestClient {

void ErrorMessage(const char* msg) {
    std::cout << msg << std::endl;
}

void WErrorMessage(const wchar_t* msg) {
    std::wcout << msg << std::endl;
}

ContentService::ContentService(const utility::string_t& serverURI, int port) 
    : connection_(serverURI, port)
{ }

pplx::task<utility::string_t> ContentService::GetBlobUUID(
    const utility::string_t serverURI,
    uint64_t size, 
    std::function<void(const char *)> errorFunc, 
    std::function<void(const wchar_t*)> wErrorFunc)
{
    http::client::http_client client(serverURI);

    auto jsonBlob = JsonCreateBlob(size);
    
    http_request request;
    request.set_method(web::http::methods::POST);
    request.set_request_uri(web::uri(U("/blob")));
    request.headers().set_content_type(U("application/vnd.api+json"));
    request.set_body(jsonBlob);

    return client.request(request)
    .then(
        [errorFunc](http_response response) -> pplx::task<web::json::value> {
            if (response.status_code() == status_codes::Created) {
                return response.extract_json(true); // ignore content-type
            }

            return pplx::task_from_result (web::json::value());
        }
    ).then(
        [errorFunc](pplx::task<web::json::value> jsonResponse) -> pplx::task<utility::string_t> {
            try {
                const auto& input = jsonResponse.get();
                if (!input.is_null()) {
                    auto responseData = input.at(U("data")).as_object();
                    utility::string_t id = responseData.at(U("id")).serialize();
                    id.pop_back();
                    id = id.substr(1U);

                    return pplx::task_from_result(id);
                }
            }
            catch (web::json::json_exception const& e) {
                errorFunc(e.what());
            }
            catch (http_exception const &e) {
                // handle error
                errorFunc(e.what());
            }

            return pplx::task_from_result(utility::string_t());
        }
    );
}

pplx::task<int64_t> ContentService::GetBlobContentLength(
    const utility::string_t serverURI,
    const utility::string_t& uuid,
    std::function<void(const char*)> errorFunc,
    std::function<void(const wchar_t*)> wErrorFunc)
{
    using Concurrency::streams::file_stream;
    using Concurrency::streams::streambuf;
    using Concurrency::streams::file_buffer;

    http_client client(serverURI);

    auto queryBlob = uri_builder();
    queryBlob.set_path(U("/blob/") + uuid);

    http_request requestBlob;
    requestBlob.set_method(web::http::methods::GET);
    requestBlob.set_request_uri(queryBlob.to_uri());

    return client.request(requestBlob).then(
    [wErrorFunc](pplx::task<web::http::http_response> previousTask) -> pplx::task<int64_t> {
        const auto& response = previousTask.get();
        const auto& responseJSON = response.extract_json(true).get();

        int64_t dataLength = -1;
        try {
            const auto& dataObj = responseJSON.at(U("data")).as_object();
            const auto& attributes = dataObj.at(U("attributes")).as_object();
            dataLength = (int64_t)attributes.at(U("contentLength")).as_integer();
        }
        catch (...) {
            wErrorFunc(responseJSON.serialize().c_str());
        }

        return pplx::task_from_result(dataLength);
    });
}

pplx::task<web::json::value> ContentService::UploadAsync(
    const utility::string_t& InputFile, 
    std::function<void(const char*)> errorFunc,
    std::function<void(const wchar_t*)> wErrorFunc,
    const pplx::cancellation_token& token /*= pplx::cancellation_token::none()*/)
{
    using Concurrency::streams::file_stream;
    using Concurrency::streams::basic_istream;

    return file_stream<uint8_t>::open_istream(InputFile).then(
    [=](pplx::task<basic_istream<uint8_t>> previousTask) -> pplx::task<web::json::value> {
        if (!token.is_canceled ()) {
            auto fileStream = previousTask.get();

            auto serverURI = connection_.GetURI();
            
            // content-length
            fileStream.seek(0, std::ios::end);
            auto dataLength = static_cast<size_t>(fileStream.tell());
            fileStream.seek(0, std::ios::beg);

            // get UUID for the blob
            return GetBlobUUID(serverURI, dataLength, errorFunc, wErrorFunc).then(
                [errorFunc, wErrorFunc, serverURI, dataLength, fileStream](pplx::task<utility::string_t> previousTask) -> pplx::task<web::json::value>
                {
                    auto uuid = previousTask.get();
                    //std::wcout << "uuid = " << uuid << std::endl;
                    if (uuid.empty()) {
                        throw http_exception(U("Failed to get UUID"));
                    }

                    http_client client(serverURI);

                    // upload file
                    // "/blob/${uuid}/upload?uploadType=resumable"
                    auto query = uri_builder();
                    query.set_path(U("/blob/") + uuid + U("/upload"));
                    query.append_query(U("uploadType"), U("resumable"));

                    http_request request;
                    request.headers().set_content_type(U("application/octet-stream"));
                    request.headers().set_content_length(dataLength);
                    request.set_request_uri(query.to_uri());
                    request.set_method(web::http::methods::PUT);
                    request.set_body(fileStream, dataLength);

                    // perform upload
                    return client.request(request).then(
                        [uuid, dataLength, fileStream, wErrorFunc](pplx::task<http_response> previousTask) -> pplx::task<web::json::value>
                        {
                            fileStream.close();

                            auto response = previousTask.get();
                            auto jsonResponse = response.extract_json().get(); // ignore content-type
                            if (response.status_code() != status_codes::OK) {
                                auto jsonResult = jsonResponse.serialize().c_str();
                                wErrorFunc(jsonResult);

                                throw http_exception(U("Failed to upload"));
                            }

                            return pplx::task_from_result(web::json::value(uuid));
                        }
                    );
                }
            );
        }

        return pplx::task_from_result(web::json::value());
    });
}

pplx::task<utility::string_t> ContentService::Upload(
    const utility::string_t& InputFile,
    std::function<void(const char*)> errorFunc,
    std::function<void(const wchar_t*)> wErrorFunc)
{
    try {
        return UploadAsync(InputFile, errorFunc, wErrorFunc).then(
            [](pplx::task<web::json::value> previousTask) -> pplx::task<utility::string_t> {
                auto response = previousTask.get();
                auto uuid = response.serialize();
                uuid.pop_back();
                uuid = uuid.substr(1U);
                return pplx::task_from_result(uuid);
            }
        );
    }
    catch (const http_exception& e) {
        errorFunc(e.what());
        return pplx::task_from_result(utility::string_t());
    }
}

pplx::task<web::json::value> ContentService::DownloadAsync(
    const utility::string_t& uuid,
    const utility::string_t& outFileName,
    std::function<void(const char*)> errorFunc,
    std::function<void(const wchar_t*)> wErrorFunc,
    const pplx::cancellation_token& token /*= pplx::cancellation_token::none()*/)
{
    using Concurrency::streams::file_stream;
    using Concurrency::streams::streambuf;
    using Concurrency::streams::file_buffer;
    
    auto serverURI = connection_.GetURI();

    return GetBlobContentLength(serverURI, uuid, errorFunc, wErrorFunc).then(
    [serverURI, uuid, outFileName, errorFunc, wErrorFunc](pplx::task<int64_t> previousTask) -> pplx::task<web::json::value> 
    {
        auto dataLength = previousTask.get();

        if (dataLength > -1) {
            http_client client(serverURI);

            auto query = uri_builder();
            query.set_path(U("/blob/") + uuid + U("/download"));
            http_request requestDownload;
            requestDownload.set_method(web::http::methods::GET);
            requestDownload.set_request_uri(query.to_uri());

            return client.request(requestDownload).then(
                [dataLength, outFileName, errorFunc, wErrorFunc](pplx::task<web::http::http_response> previousTask) -> pplx::task<web::json::value>
                {
                    auto response = previousTask.get();
                    auto responseJSON = response.extract_json().get();
                    if (response.status_code() != status_codes::OK) {
                        wErrorFunc(responseJSON.serialize().c_str());

                        return pplx::task_from_result(web::json::value(CONTENT_SERVICE_TASK_FAIL));
                    }

                    int64_t downloadDataLength = -1;
                    try {
                        streambuf<uint8_t> localFile = file_buffer<uint8_t>::open(outFileName).get();
                        return response.body().read_to_end(localFile).then(
                            [dataLength, &downloadDataLength](pplx::task<size_t> previousTask) -> pplx::task<web::json::value>
                            {
                                downloadDataLength = previousTask.get();

                                if (downloadDataLength != dataLength) {
                                    throw http_exception(U("contentLength mismatched!"));
                                }

                                return pplx::task_from_result(web::json::value(downloadDataLength));
                            }
                        );
                    }
                    catch (const http_exception& e) {
                        errorFunc(e.what());
                    }
                    catch (...) {
                        // generic exception
                    }

                    return pplx::task_from_result(web::json::value(downloadDataLength));
                }
            );
        }
        else {
            return pplx::task_from_result(web::json::value());
        }
    });
}

pplx::task<web::json::value> ContentService::DownloadAsync(
    const utility::string_t& uuid,
    uint8_t* data,
    std::function<void(const char*)> errorFunc,
    std::function<void(const wchar_t*)> wErrorFunc,
    const pplx::cancellation_token& token /*= pplx::cancellation_token::none()*/)
{
    using Concurrency::streams::file_stream;
    using Concurrency::streams::streambuf;
    using Concurrency::streams::file_buffer;

    auto serverURI = connection_.GetURI();

    auto outData = std::make_shared<uint8_t*>(data);

    return GetBlobContentLength(serverURI, uuid, errorFunc, wErrorFunc).then(
        [serverURI,uuid,outData,errorFunc,wErrorFunc](pplx::task<int64_t> previousTask) -> pplx::task<web::json::value> 
        {
            auto dataLength = previousTask.get();

            if (dataLength > -1) {
                http_client client(serverURI);

                auto query = uri_builder();
                query.set_path(U("/blob/") + uuid + U("/download"));
                http_request requestDownload;
                requestDownload.set_method(web::http::methods::GET);
                requestDownload.set_request_uri(query.to_uri());

                return client.request(requestDownload).then(
                    [dataLength,outData,errorFunc,wErrorFunc](pplx::task<web::http::http_response> previousTask) -> pplx::task<web::json::value>
                    {
                        auto response = previousTask.get();
                        auto responseJSON = response.extract_json().get();
                        if (response.status_code() != status_codes::OK) {
                            wErrorFunc(responseJSON.serialize().c_str());

                            return pplx::task_from_result(web::json::value(CONTENT_SERVICE_TASK_FAIL));
                        }

                        int64_t downloadDataLength = -1;
                        try {
                            *outData = new uint8_t[dataLength];
                            rawptr_buffer<uint8_t> rawOutputBuffer(*outData, dataLength);
                            return response.body().read_to_end(rawOutputBuffer).then(
                                [dataLength, &downloadDataLength](pplx::task<size_t> previousTask) -> pplx::task<web::json::value>
                            {
                                downloadDataLength = previousTask.get();

                                if (downloadDataLength != dataLength) {
                                    throw http_exception(U("contentLength mismatched!"));
                                }

                                return pplx::task_from_result(web::json::value(downloadDataLength));
                            }
                            );
                        }
                        catch (const http_exception& e) {
                            errorFunc(e.what());
                        }
                        catch (...) {
                            // generic exception
                        }

                        return pplx::task_from_result(web::json::value(downloadDataLength));
                    }
                );
            } else {
                return pplx::task_from_result(web::json::value());
            }
        }
    );
}

int64_t ContentService::Download(
    const utility::string_t& uuid,
    const utility::string_t& outFileName,
    std::function<void(const char*)> errorFunc,
    std::function<void(const wchar_t*)> wErrorFunc)
{
    int64_t dataLength = -1;
    try {
        DownloadAsync(uuid, outFileName, errorFunc, wErrorFunc).then(
            [&dataLength](pplx::task<web::json::value> previousTask) -> int64_t {
                auto result = previousTask.get();
                dataLength = result.as_integer();
                return dataLength;
            }
        ).wait();

        return dataLength;
    }
    catch (http_exception const& e) {
        errorFunc(e.what());

        return CONTENT_SERVICE_TASK_FAIL;
    }
}

int64_t ContentService::Download(
    const utility::string_t& uuid,
    uint8_t* data,
    std::function<void(const char*)> errorFunc,
    std::function<void(const wchar_t*)> wErrorFunc)
{
    int64_t dataLength = -1;
    try {
        DownloadAsync(uuid, data, errorFunc, wErrorFunc).then(
            [&dataLength](pplx::task<web::json::value> previousTask) -> int64_t {
            auto result = previousTask.get();
            dataLength = result.as_integer();
            return dataLength;
        }
        ).wait();
    }
    catch (http_exception const& e) {
        errorFunc(e.what());
    }

    return CONTENT_SERVICE_TASK_FAIL;
}

} // namespace TestClient
