cmake_minimum_required(VERSION 3.15)

# Set CMake policy for Boost finding (if available)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Only set up the project if we're not being included by another project
if(NOT PROJECT_NAME)
    project(mse_solver)
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
add_executable(mse_solver_main
    main.cpp
    mse_solver.cpp
)

# Link with the library
target_link_libraries(mse_solver_main ggg)

# Set output name to match solver naming convention
set_target_properties(mse_solver_main PROPERTIES OUTPUT_NAME "ggg_mse")

# Also create the library for testing and backwards compatibility
add_library(mse_solver 
    mse_solver.cpp
)

target_include_directories(mse_solver PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mse_solver PUBLIC
    ggg
)

# Set C++20 standard
target_compile_features(mse_solver PUBLIC cxx_std_20)

# Add to parent target if it exists
if(TARGET solvers)
    target_link_libraries(solvers INTERFACE mse_solver)
endif()
