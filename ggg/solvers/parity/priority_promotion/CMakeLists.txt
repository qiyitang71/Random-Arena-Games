cmake_minimum_required(VERSION 3.15)

# Set CMake policy for Boost finding (if supported)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Only set up the project if we're not being included by another project
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(priority_promotion_parity_solver)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages if we're not part of the main project
if(NOT TARGET ggg)
    find_package(Boost REQUIRED COMPONENTS graph program_options)
    
    # Find the ggg library
    find_path(GGG_INCLUDE_DIR 
        NAMES libggg/libggg.hpp
        PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
        NO_DEFAULT_PATH
    )
    
    if(NOT GGG_INCLUDE_DIR)
        message(FATAL_ERROR "Could not find GGG include directory")
    endif()
    
    # Create an interface library for ggg if it doesn't exist
    add_library(ggg INTERFACE)
    target_include_directories(ggg INTERFACE ${GGG_INCLUDE_DIR})
    target_link_libraries(ggg INTERFACE Boost::graph Boost::program_options)
endif()

# Create the solver library
add_library(priority_promotion_solver_lib STATIC
    priority_promotion_solver.cpp
    priority_promotion_solver.hpp
)

target_link_libraries(priority_promotion_solver_lib 
    PUBLIC ggg
    PRIVATE Boost::graph
)

target_include_directories(priority_promotion_solver_lib 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# Create the solver executable
add_executable(priority_promotion_solver
    main.cpp
)

target_link_libraries(priority_promotion_solver 
    PRIVATE priority_promotion_solver_lib
    PRIVATE ggg
    PRIVATE Boost::program_options
)

# Set output directory to match GGG structure
set_target_properties(priority_promotion_solver PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/solvers/parity/priority_promotion
)

# Install targets (if we're part of the main build)
if(TARGET ggg)
    install(TARGETS priority_promotion_solver
        RUNTIME DESTINATION bin/solvers/parity/priority_promotion
    )
endif()
