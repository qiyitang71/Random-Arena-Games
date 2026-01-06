cmake_minimum_required(VERSION 3.15)

# Set CMake policy for Boost finding (if supported)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Only set up the project if we're not being included by another project
if(NOT PROJECT_NAME)
    project(reachability_game_solver)
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

# Find Boost test framework for testing
find_package(Boost REQUIRED COMPONENTS unit_test_framework)

# Create the solver executable
add_executable(ggg_reachability_solver 
    main.cpp 
    reachability_solver.cpp
)

# Link against the main library and Boost
target_link_libraries(ggg_reachability_solver 
    PRIVATE 
        ggg
        Boost::graph 
        Boost::program_options
)

# Set compiler flags
target_compile_options(ggg_reachability_solver PRIVATE 
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Set include directories
target_include_directories(ggg_reachability_solver PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

# Set output name to match solver naming convention
set_target_properties(ggg_reachability_solver PROPERTIES OUTPUT_NAME "ggg_reachability")

# Also create the library for testing and backwards compatibility
add_library(reachability_solver 
    reachability_solver.cpp
)

target_include_directories(reachability_solver PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(reachability_solver PUBLIC
    ggg
)

# Set C++20 standard
target_compile_features(reachability_solver PUBLIC cxx_std_20)

# Add to parent target if it exists
if(TARGET solvers)
    target_link_libraries(solvers INTERFACE reachability_solver)
endif()

# Add tests if BUILD_TESTING is enabled
if(BUILD_TESTING)    
    add_executable(test_reachability_solver
        test_reachability_solver.cpp
        reachability_solver.cpp
    )
    
    target_link_libraries(test_reachability_solver
        PRIVATE
            ggg
            Boost::unit_test_framework
    )
    
    target_include_directories(test_reachability_solver PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    # Set C++20 standard
    target_compile_features(test_reachability_solver PRIVATE cxx_std_20)
    
    # Add test to CTest
    add_test(NAME reachability_solver_tests COMMAND test_reachability_solver)
endif()
