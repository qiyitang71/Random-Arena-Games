cmake_minimum_required(VERSION 3.15)
project(libggg_tools)

# Set CMake policy for Boost finding
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Boost REQUIRED COMPONENTS program_options filesystem system CONFIG)

# Check if we're building standalone or as part of main project
if(NOT TARGET ggg)
    # We're building standalone - create a minimal interface for the ggg tool
    add_library(ggg_standalone INTERFACE)
    target_include_directories(ggg_standalone INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    
    # Set minimal logging configuration for standalone build
    target_compile_definitions(ggg_standalone INTERFACE ENABLE_LOGGING LOG_LEVEL=3)
    
    # Link Boost libraries (program_options only since graph is header-only)
    target_link_libraries(ggg_standalone INTERFACE Boost::program_options)
    
    set(GGG_TARGET ggg_standalone)
else()
    # We're part of the main project
    set(GGG_TARGET ggg)
endif()

# Create a library for tool implementations
add_library(ggg_tools_lib 
    benchmark_solvers.cpp
    generate_parity_games.cpp
    generate_mpv_games.cpp
    generate_discounted_games.cpp
    generate_games.cpp
    list_solvers.cpp
)
target_link_libraries(ggg_tools_lib ${GGG_TARGET} Boost::program_options Boost::filesystem Boost::system)
target_include_directories(ggg_tools_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_compile_definitions(ggg_tools_lib PRIVATE GGG_NO_MAIN)

# Main GGG tool
add_executable(ggg_tool ggg_main.cpp)
target_link_libraries(ggg_tool ggg_tools_lib ${GGG_TARGET} Boost::program_options Boost::filesystem Boost::system)
target_include_directories(ggg_tool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Set output directory
set_target_properties(ggg_tools_lib ggg_tool
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Set the output name for the main tool to be "ggg"
set_target_properties(ggg_tool PROPERTIES OUTPUT_NAME ggg)
