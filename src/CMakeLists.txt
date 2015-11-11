include_directories(
    ../include
    ${CASABLANCA_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
)

# # platform specific settings
# if(UNIX)
# 	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNINGS} -Werror -pedantic")
# elseif(WIN32)
# 	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNINGS} /Yustdafx.h /Zm200")
# 	SET_SOURCE_FILES_PROPERTIES(pch/stdafx.cpp PROPERTIES COMPILE_FLAGS "/Ycstdafx.h")
# endif()

set(HEADERS
    ../include/miscutils.h
    ../include/jsonutils.h
    ../include/testinputstream.h
    ../include/testparameters.h
    ../include/contentservice.h)

set(SOURCES
    miscutils.cpp
    jsonutils.cpp
    testinputstream.cpp
    testparameters.cpp
    contentservice.cpp
    main.cpp)

SET(ADDITIONAL_LIBRARIES ${CASABLANCA_LIBRARY} ${Boost_LIBRARIES})

LINK_DIRECTORIES($ENV{BOOST_LIBDIR})
MESSAGE("BOOST_LIBDIR = $ENV{BOOST_LIBDIR}")

add_executable(testcpprestsdkmain ${SOURCES} ${HEADERS})
target_link_libraries(testcpprestsdkmain ${ADDITIONAL_LIBRARIES})

MESSAGE("Boost_LIBRARIES = ${Boost_LIBRARIES}")


cotire(testcpprestsdkmain)
