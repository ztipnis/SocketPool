cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME SocketPool)
set(VERSION_NUM 1.0)
project(${PROJECT_NAME} VERSION ${VERSION_NUM})

# specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
#Set output Directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


add_subdirectory(Pollster)


add_library(SocketPool STATIC SocketPool.cpp)
add_dependencies(SocketPool pollster)

#Find Threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


target_link_libraries(SocketPool pollster Threads::Threads)

install(TARGETS SocketPool DESTINATION lib)
install(FILES SocketPool.hpp DESTINATION include)
