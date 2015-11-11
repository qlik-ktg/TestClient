#include "testparameters.h"
#include "contentservice.h"
#include "miscutils.h"

#include <ppltasks.h>
#include <array>
#include <string>
#include <iostream>
#include <chrono>

#if _WIN32
#include <conio.h>
#endif

using namespace std;
using namespace concurrency;
using namespace TestClient;

void GetInputDataFiles(const utility::string_t& dataPath, 
                       const std::vector<utility::string_t>& fileNames, 
                       std::vector<utility::string_t>& dataFiles) 
{
    utility::stringstream_t ss;
    dataFiles.resize(0);
    for (auto i = 0; i < fileNames.size(); ++i) {
        ss.str(utility::string_t());
        ss << dataPath << "//" << fileNames[i];
        dataFiles.push_back(ss.str());
    }
}

void GetOutputDataFiles(int taskId,
                        const utility::string_t& dataPath,
                        const std::vector<utility::string_t>& fileNames,
                        std::vector<utility::string_t>& dataFiles)
{
    utility::stringstream_t ss;
    dataFiles.resize(0);
    for (auto i = 0; i < fileNames.size(); ++i) {
        ss.str(utility::string_t());
        ss << dataPath << U("//task") << taskId << U("_download_") << fileNames[i];
        dataFiles.push_back(ss.str());
    }
}

utility::string_t TestUpload(const utility::string_t& dataFile,
               const utility::string_t& server,
               int port,
               const int taskId)
{
    ContentService service(server, port);

    auto errorFunc = [](const char* msg) { ErrorMessage(msg); };
    auto wErrorFunc = [](const wchar_t* msg) { WErrorMessage(msg); };

    auto tStart = std::chrono::high_resolution_clock::now();
    auto uuid = service.Upload(dataFile, errorFunc, wErrorFunc).get();
    auto tStop = std::chrono::high_resolution_clock::now();
    auto timeMS = std::chrono::duration_cast<std::chrono::milliseconds>(tStop - tStart).count();

    auto fileLength = GetFileSize(dataFile);

    utility::stringstream_t ss;
    if (!uuid.empty()) {
        ss << U("Task ") << taskId 
            << U(", upload file ") << dataFile
            << U(" in ") << timeMS << U("ms. ") 
            << ((double)fileLength / (double)timeMS / 1000.0) << U("MB/s")<< std::endl;
    }
    else {
        ss << "Failed to upload file " << dataFile << std::endl;
        std::wcout << ss.str();
    }

    std::wcout << ss.str();

    return uuid;
}

int TestDownload(const utility::string_t& uuid,
                 const utility::string_t& server,
                 int port,
                 const utility::string_t& dataPath,
                 int taskId)
{
    ContentService service(server, port);
    auto errorFunc = [](const char* msg) { ErrorMessage(msg); };
    auto wErrorFunc = [](const wchar_t* msg) { WErrorMessage(msg); };

    utility::stringstream_t ss;
    ss << dataPath << U("//") << taskId << uuid << ".bin";

    auto tStart = std::chrono::high_resolution_clock::now();
    auto contentLength = service.Download(uuid, ss.str(), errorFunc, wErrorFunc);
    auto tStop = std::chrono::high_resolution_clock::now();
    auto timeMS = std::chrono::duration_cast<std::chrono::milliseconds>(tStop - tStart).count();

    ss.str(utility::string_t());
    if (contentLength > 0) {
        ss << U("Task ") << taskId
            << U(", download blob ") << uuid
            << U(" in ") << timeMS << U("ms. ")
            << ((double)contentLength / (double)timeMS / 1000.0) << U("MB/s") << std::endl;
    }
    else {
        ss << "Failed to download blob " << uuid << std::endl;
    }

    std::wcout << ss.str();

    return 0;
}

int TestUploadThreads(const std::vector<utility::string_t>& dataFiles, const utility::string_t& server, int port, size_t numTasks) {
    pplx::task_group tg;
    for (auto i = 0; i < numTasks; ++i) {
        for (auto j = 0; j < dataFiles.size(); ++j) {
            auto dataFile = dataFiles[j];
            tg.run(
                [=]() -> utility::string_t {
                return TestUpload(dataFile, server, port, i);
            });
        }
    }
    tg.wait();

    std::wcout << U("Finished!") << std::endl;

    return 0;
}

int TestUploadAndDownloadThreads(const std::vector<utility::string_t>& dataFiles, const utility::string_t& server, int port, const utility::string_t& dataPath, size_t numTasks) {
    pplx::task_group tg;

    // upload files
    std::vector<pplx::task<utility::string_t>> uploadTasks;
    for (auto i = 0; i < numTasks; ++i) {
        for (auto j = 0; j < dataFiles.size(); ++j) {
            auto dataFile = dataFiles[j];
            uploadTasks.push_back(
                create_task(
                [dataFile,server,port,i]() -> utility::string_t {
                    return TestUpload(dataFile, server, port, i);
                })
            );
        }
    }

    auto pUploadUUIDs = std::make_shared<std::vector<utility::string_t>>(std::vector<utility::string_t>());
    auto joinUploadTasks = when_all(begin(uploadTasks), end(uploadTasks)).then(
        [pUploadUUIDs](std::vector<utility::string_t> uploadResults) {
            for (auto i = 0; i < uploadResults.size(); ++i) {
                const auto& uuid = uploadResults[i];
                pUploadUUIDs->push_back(uuid);

                //std::wcout << U("uuid = ") << uuid << std::endl;
            }
        }
    );
    joinUploadTasks.wait();

    std::vector<pplx::task<int>> downloadTasks;
    for (auto j = 0; j < pUploadUUIDs->size(); ++j) {
        const auto& uuid = (*pUploadUUIDs)[j];
        downloadTasks.push_back(
            create_task (
            [uuid,server,port,dataPath,j]() -> int {
                return TestDownload(uuid, server, port, dataPath, j);
            })
        );
    }

    auto joinDownloadTask = when_all(begin(downloadTasks), end(downloadTasks));
    joinDownloadTask.wait();

    std::wcout << U("Finished!") << std::endl;

    return 0;
}

int main(int argc, char** argv) {

	cout << "Test CppRestSDK" << endl;

    if (argc < 3) {
        cout << "Usage:" << endl
            << "TestClient <config_file> <testMode>" 
            << endl
            << "testMode: 0 = upload, 1 = download, 2 = upload and download"
            << endl;
        return -1;
    }

    std::istringstream iss(argv[2]);
    int testMode = 0;
    iss >> testMode;

	std::string configPath(argv[1]);
    TestParameters testParams(configPath);
    testParams.Parse();

    auto numTasks = testParams.NumInstances();
    const auto& server = testParams.Server();
    const auto& port = testParams.Port();
    const auto& dataPath = testParams.DataPath();
    const auto& fileNames = testParams.FileNames();
    std::vector<utility::string_t> dataFiles;
    GetInputDataFiles(dataPath, fileNames, dataFiles);

    switch (testMode) {
    case 0: // upload
        TestUploadThreads(dataFiles, server, port, numTasks);
        break;
    case 1: // download
        cout << "Not implemented yet!" << endl;
        break;

    case 2: // upload and download
        TestUploadAndDownloadThreads(dataFiles, server, port, dataPath, numTasks);
        break;
    }

#if _WIN32
    cout << "Press any key to continue...";
    _getch();
#endif // _WIN32

	return 0;
}
