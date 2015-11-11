#include "testparameters.h"

#include "cpprest/json.h"
#include "cpprest/filestream.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace std;
using namespace web;
using namespace web::json;

namespace TestClient {

TestParameters::TestParameters(const std::string& filePath) 
		: filePath_(filePath)
{ }

const utility::string_t& TestParameters::Server() const {
    return server_;
}

utility::string_t& TestParameters::Server() {
    return server_;
}

const utility::string_t& TestParameters::ServerURI() const {
    return serverURI_;
}

utility::string_t& TestParameters::ServerURI() {
    return serverURI_;
}

size_t TestParameters::NumInstances() {
    return numInstances_;
}

int TestParameters::Scenario() {
    return scenarioType_;
}

//const std::vector<int>& TestParameters::DataSizes() const {
//    return dataSize_;
//}
//
//std::vector<int>& TestParameters::DataSizes()
//{
//    return dataSize_;
//}

void TestParameters::Parse() {
    std::ifstream ifs;
    ifs.open(filePath_.c_str(), std::ifstream::in);
    if (ifs) {
        try {
            auto testParams = web::json::value::parse(ifs);

            // get server name and remove quote marks
            server_ = testParams.at(U("server")).serialize();
            server_.pop_back();
            server_ = server_.substr(1U);

            // data path, remove quotes and the last backslash
            dataPath_ = testParams.at(U("dataPath")).serialize();
            dataPath_.pop_back();
            dataPath_ = dataPath_.substr(1U);
            
            port_ = testParams.at(U("port")).as_integer();
            numInstances_ = testParams.at(U("numInstances")).as_integer();

            const auto& TestScenario = testParams.at(U("scenario")).as_object();
            scenarioType_ = TestScenario.at(U("type")).as_integer();
            const auto& Files = TestScenario.at(U("files")).as_array();
            for (auto iter = Files.cbegin(); iter != Files.cend(); ++iter) {
                auto file = iter->at(U("file")).serialize();
                file.pop_back();
                file = file.substr(1U);
                dataFiles_.push_back(file);
            }

            serverURI_ = server_;
            if (port_ != 0) {
                utility::stringstream_t stream;
                stream << server_ << ":" << port_;
                serverURI_ = stream.str();
            }
        }
        catch (const std::exception& e) {
            std::string reason = e.what();
            std::cout << reason << std::endl;
        }
        ifs.close();
    }
}

}