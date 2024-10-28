cmake_minimum_required(VERSION 3.13)

project(bittorrent-client)

# Set C++ standards
# set(CMAKE_CXX_STANDARD 23)

# Recursively get all .cpp and .hpp files from the src directory
file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp src/*.h)

# create executable with the collected source files
add_executable(bittorrent ${SOURCE_FILES})
