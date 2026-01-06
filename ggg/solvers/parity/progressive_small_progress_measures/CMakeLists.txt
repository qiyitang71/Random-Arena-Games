cmake_minimum_required(VERSION 3.15)

# Set CMake policy for Boost finding
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Only set up the project if we're not being included by another project
if(NOT PROJECT_NAME)
    project(progressive_small_progress_measures_parity_solver)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages if we're not part of the main project
if(NOT TARGET ggg)
    find_package(Boost REQUIRED COMPONENTS graph program_options)
    
    # Add the main library as a subdirectory relative to this location
    add_subdirectory(../../../)
endif()

# Create the solver executable
add_executable(ggg_progressive_small_progress_measures_parity_solver 
    main.cpp 
    progressive_small_progress_measures_solver.cpp
)

# Link with the library
target_link_libraries(ggg_progressive_small_progress_measures_parity_solver ggg)

# Include current directory for local header
target_include_directories(ggg_progressive_small_progress_measures_parity_solver PRIVATE .)

# Set output name to match solver naming convention
set_target_properties(ggg_progressive_small_progress_measures_parity_solver PROPERTIES OUTPUT_NAME "ggg_progressive_small_progress_measures")
