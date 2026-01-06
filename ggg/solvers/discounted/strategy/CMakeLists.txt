cmake_minimum_required(VERSION 3.15)

# Set CMake policy for Boost finding (if supported)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Find Boost components
find_package(Boost REQUIRED COMPONENTS graph program_options CONFIG)

# Create executable for discounted strategy improvement solver
add_executable(discounted_strategy_solver 
    main.cpp
    discounted_strategy_solver.cpp
)

# Link with the main library and Boost
target_link_libraries(discounted_strategy_solver 
    ggg
    Boost::graph 
    Boost::program_options
)

# Include directories
target_include_directories(discounted_strategy_solver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Set C++20 standard
target_compile_features(discounted_strategy_solver PRIVATE cxx_std_20)

# Also create the library for testing and backwards compatibility
add_library(discounted_strategy_solver_lib 
    discounted_strategy_solver.cpp
)

target_include_directories(discounted_strategy_solver_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(discounted_strategy_solver_lib PUBLIC
    ggg
    Boost::graph 
    Boost::program_options
)

# Set C++20 standard
target_compile_features(discounted_strategy_solver_lib PUBLIC cxx_std_20)

# Add to parent target if it exists
if(TARGET solvers)
    target_link_libraries(solvers INTERFACE discounted_strategy_solver_lib)
endif()