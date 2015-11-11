cmake_minimum_required(VERSION 2.8)
project(test_cpprestsdk)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(cotire)

# static linking
SET(BUILD_SHARED_LIBS OFF)

FIND_PACKAGE(Casablanca REQUIRED)
FIND_PACKAGE(BOOST REQUIRED COMPONENTS system iostreams)

# Platform specific settings
# currently, nothing here

# Compiler specific settings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	message("-- Setting gcc options")
	set(WARNINGS "-Wall -Wextra -Wunused-parameter -Wcast-align -Wcast-qual -Wconversion -Wformat=2 -Winit-self -Winvalid-pch -Wmissing-format-attribute -Wmissing-include-dirs -Wpacked -Wredundant-decls -Wunreachable-code")
  	set(LD_FLAGS "${LD_FLAGS} -Wl,-z,defs")
  	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-strict-aliasing -static")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	message("-- Setting msvc options")
	set(WARNINGS)
else()
	message("-- Unknown compiler, finger crossed!!!")
	message("CMAKE_CXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID}")
endif()

# output make_directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Binaries)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Binaries)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Binaries)

add_subdirectory(src)

if (BUILD_TESTS)
	add_subdirectory(tests)
endif()
