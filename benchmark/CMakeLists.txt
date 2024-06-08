# Set up CMake project and minimum version.
cmake_minimum_required(VERSION 3.13)
set(target benchmark)
project(${target} VERSION 1.0 LANGUAGES CXX)

# Set C++ standard to C++20.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Uncomment this line when building on ARM architectures.
# set(CMAKE_OSX_ARCHITECTURES "arm64")

# Specify the executable.
add_executable(${target}
    src/packet.cpp
    src/siphash.cpp
    src/globalipid.cpp
    src/perconnipid.cpp
    src/perdestipid.cpp
    src/perbucketlipid.cpp
    src/perbucketmipid.cpp
    src/prngqueueipid.cpp
    src/prngshuffleipid.cpp
    src/perbucketshuffleipid.cpp
    src/main.cpp)

# Specify include directories.
include_directories(include)
include_directories(${Boost_INCLUDE_DIRS})

# Get necessary Boost libraries.
find_package(Boost REQUIRED COMPONENTS program_options)

# Perform linking.
target_link_libraries(${target} ${Boost_LIBRARIES})
target_link_libraries(${target} pthread)
