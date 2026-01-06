cmake_minimum_required(VERSION 3.15)

# Set CMake policy for Boost finding (if supported)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Find Boost components
find_package(Boost REQUIRED COMPONENTS graph program_options CONFIG)

# Create executable for stochastic discounted objective solver
add_executable(stochastic_discounted_objective_solver 
    main.cpp
    stochastic_discounted_objective_solver.cpp
)

# Link with the main library and Boost
target_link_libraries(stochastic_discounted_objective_solver 
    ggg
    Boost::graph 
    Boost::program_options
)

# Include directories
target_include_directories(stochastic_discounted_objective_solver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Set C++20 standard
target_compile_features(stochastic_discounted_objective_solver PRIVATE cxx_std_20)

# Also create the library for testing and backwards compatibility
add_library(stochastic_discounted_objective_solver_lib 
    stochastic_discounted_objective_solver.cpp
)

target_include_directories(stochastic_discounted_objective_solver_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(stochastic_discounted_objective_solver_lib PUBLIC
    ggg
    Boost::graph 
    Boost::program_options
)

# Set C++20 standard
target_compile_features(stochastic_discounted_objective_solver_lib PUBLIC cxx_std_20)

# Add to parent target if it exists
if(TARGET solvers)
    target_link_libraries(solvers INTERFACE stochastic_discounted_objective_solver_lib)
endif()