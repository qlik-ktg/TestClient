#pragma once

#include "cpprest/json.h"
#include "cpprest/streams.h"
#include <string>

namespace TestClient {

enum TestScenario {
    SCENARIOS_UPLOAD = 0,
    SCENARIOS_DOWNLOAD,
    NUM_SCENARIOS
};

class TestParameters {

    std::string                     filePath_;
    utility::string_t               serverURI_;
    utility::string_t               server_;
    utility::string_t               dataPath_;
    int                             port_;
    size_t                          numInstances_;
    int                             scenarioType_;
    //std::vector<int>                dataSize_;
    std::vector<utility::string_t>  dataFiles_;


public:
    TestParameters() = delete;
	TestParameters(const std::string& filePath);

	~TestParameters() { }

	void Parse();

    const utility::string_t& Server() const;
    utility::string_t& Server();
    
    const int& Port() const { return port_; }
    int& Port() { return port_; }

    const utility::string_t& DataPath() const { return dataPath_; }
    utility::string_t& DataPath() { return dataPath_; }
    
    const utility::string_t& ServerURI() const;
    utility::string_t& ServerURI();
    
    size_t NumInstances();
    int Scenario();

    const std::vector<utility::string_t>& FileNames() const { return dataFiles_; }
    std::vector<utility::string_t>& FileNames() { return dataFiles_; }

    /*const std::vector<int>& DataSizes() const;
    std::vector<int>& DataSizes();*/
}; // TestParameters

}
