#TestClient

Test client is intended to test the performance of content service.


Compile and Build
-----------------------------
Test client is configured using cmake and depends on Boost and cpprestsdk.

1. Download and compile cpprestsdk source (https://github.com/Microsoft/cpprestsdk)
2. Set the environment variables
    - CASABLANCA_DIR to the path to the library, i.e., <source_folder>/cpprestsdk
3. Install Boost which is required for CRC32 implementation
