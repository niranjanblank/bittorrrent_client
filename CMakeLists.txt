
cmake_minimum_required(VERSION 3.13)

project(bittorrent-client)

# Set C++ standards
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Recursively get all .cpp and .hpp files from the src directory
file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp src/*.h)

# Print the list of source files for verification
message(STATUS "SOURCE_FILES: ${SOURCE_FILES}")

# Create executable with the collected source files
add_executable(bittorrent ${SOURCE_FILES})

