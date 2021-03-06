cmake_minimum_required(VERSION 2.8.4)
project(IO)
set(VERSION_MAJOR "0")
set(VERSION_MINOR "2")
set(VERSION_PATCH "19")
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(DESCRIPTION "Common UNIX IO library for C++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11  -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -O3 -march=native")

include(CMake-install-headers.txt)

set(SOURCE_FILES  src/io.cpp src/async.cpp src/application.cpp src/serial.cpp)
set(HEADERS_LIST  ${IO_HEADERS})
set(RUNTIME_DEPS )

if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]")
    message("debug mode")
    set(VERSION "${VERSION}-debug")
endif()

if(EXPERIMENTAL)
    list(APPEND SOURCE_FILES src/experimental.cpp)
endif()

# Shared library
add_library(${PROJECT_NAME}-SharedLib SHARED ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME}-SharedLib PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
target_compile_definitions(${PROJECT_NAME}-SharedLib PUBLIC BUILD_VERSION="${VERSION}")
target_link_libraries(${PROJECT_NAME}-SharedLib ${LIBS})

# Static library
add_library(${PROJECT_NAME}-StaticLib STATIC ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME}-StaticLib PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
target_compile_definitions(${PROJECT_NAME}-StaticLib PUBLIC BUILD_VERSION="${VERSION}")
target_link_libraries(${PROJECT_NAME}-StaticLib ${LIBS})

IO_INSTALL_HEADERS(/usr/include/io/)
install(TARGETS ${PROJECT_NAME}-SharedLib ${PROJECT_NAME}-StaticLib DESTINATION /usr/lib/)

# Setup DEBIAN control files
set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE 1)
set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
set(CPACK_DEBIAN_PACKAGE_DEPENDS ${RUNTIME_DEPS})
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${DESCRIPTION})
set(CPACK_PACKAGE_DESCRIPTION "https://github.com/reddec/io")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Red Dec <net.dev@mail.ru>")

include(CPack)