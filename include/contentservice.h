#pragma once

#include "testparameters.h"

#include "cpprest/http_client.h"
#include "cpprest/streams.h"

namespace TestClient {

const utility::string_t STRING_UPLOAD_FAILED(U("Upload failed"));
const utility::string_t STRING_UPLOAD_SUCCESS(U("Upload success"));
const utility::string_t STRING_DOWNLOAD_FAILED(U("Download failed"));
const utility::string_t STRING_DOWNLOAD_SUCCESS(U("Download success"));
const utility::string_t STRING_GET_BLOB_FAILED(U("Get data blob failed"));
const utility::string_t STRING_GET_BLOB_SUCCESS(U("Get data blob success"));

static const int CONTENT_SERVICE_TASK_SUCCESS = 1;
static const int CONTENT_SERVICE_TASK_FAIL = -1;

/**
 * \brief Information for connecting to a content service instance
 */
struct ContentServiceConnection {
    utility::string_t   serverURI_;
    int                 port_;

    ContentServiceConnection() 
        : serverURI_(U("")),
        port_(80)
    { }
    ContentServiceConnection(const utility::string_t& serverURI, int port)
        : serverURI_(serverURI), port_(port)
    { }

    utility::string_t GetURI() const {
        utility::stringstream_t ss;
        ss << serverURI_ << U(":") << port_;
        
        return ss.str();
    }
}; // ContentServiceConnection

/**
 * \brief Output error message
 */
void ErrorMessage(const char*);
void WErrorMessage(const wchar_t*);

// TODO: - resumable upload

/**
 * \brief Handles uploading and downloading from content service
 */
class ContentService {
    ContentServiceConnection    connection_;

    /**
     * \brief Creates a blob structure and get UUID
     * @param size  size of the blob
     * @return uuid
     */
    pplx::task<utility::string_t> GetBlobUUID(
        const utility::string_t serverURI,
        uint64_t size,
        std::function<void(char const *)> error,
        std::function<void(const wchar_t*)> wErrorFunc);

    /**
     * \brief Gets the length of the data blob
     *
     * @param uuid
     * @param content-length of the blob
     */
    pplx::task<int64_t> GetBlobContentLength(
        const utility::string_t serverURI,
        const utility::string_t& uuid,
        std::function<void(const char*)> errorFunc,
        std::function<void(const wchar_t*)> wErrorFunc);

    /**
     * \brief Async upload a file to content service
     * @param connection
     * @param filename
     * @param errorFunc
     * @param wErrorFunc
     * @param token
     * @return success: the uuid string of the uploaded blob; fail: an empty string
     */
    pplx::task<web::json::value> UploadAsync(
        const utility::string_t& filename,
        std::function<void(const char*)> errorFunc,
        std::function<void(const wchar_t*)> wErrorFunc,
        const pplx::cancellation_token& token = pplx::cancellation_token::none());

    /**
     * \brief Async download a file from content service
     * @param connection
     * @param uuid
     * @param outFileName
     * @param errorFunc
     * @param wErrorFunc
     * @param token
     * @return success : the size of the downloaded data; fail : null json object
     */
    pplx::task<web::json::value> DownloadAsync(
        const utility::string_t& uuid,
        const utility::string_t& outFileName,
        std::function<void(const char*)> errorFunc,
        std::function<void(const wchar_t*)> wErrorFunc,
        const pplx::cancellation_token& token = pplx::cancellation_token::none());

    /**
     * \brief Async download a file from content service
     * @param connection
     * @param uuid
     * @param outFileName
     * @param errorFunc
     * @param wErrorFunc
     * @param token
     * @return success : the size of the downloaded data; fail : null json object
     */
    pplx::task<web::json::value> DownloadAsync(
        const utility::string_t& uuid,
        uint8_t* data,
        std::function<void(const char*)> errorFunc,
        std::function<void(const wchar_t*)> wErrorFunc,
        const pplx::cancellation_token& token = pplx::cancellation_token::none());

public:
    ContentService() = delete;
    ContentService(const utility::string_t& serverURI, int port);

    /**
     * \brief Upload a file to content service
     * @param fileName
     * @param error
     * @return file size : success, -1 : fail
     * @return uuid
     */
    pplx::task<utility::string_t> Upload(
        const utility::string_t& fileName,
        std::function<void(const char*)> errorFunc,
        std::function<void(const wchar_t*)> wErrorFunc);

    /**
     * \brief Download data from content service to a file
     * @param uuid
     * @param outFileName   Name of the file storing downloaded data
     * @param errorFunc
     * @param wErrorFunc
     * @return file size : success, -1 : fail
    */
    int64_t Download(
        const utility::string_t& uuid,
        const utility::string_t& outFileName,
        std::function<void(const char*)> errorFunc,
        std::function<void(const wchar_t*)> wErrorFunc);

    /**
     * \brief Download from content service to a data buffer
     * @param uuid
     * @param data  Downloaded data
     * @param errorFunc
     * @param wErrorFunc
     * @return file size : success, -1 : fail
     */
    int64_t Download(
        const utility::string_t& uuid,
        uint8_t* data,
        std::function<void(const char*)> errorFunc,
        std::function<void(const wchar_t*)> wErrorFunc);
}; // ContentService

} // namespace TestClient
