cmake_minimum_required(VERSION 3.15)
project(libggg VERSION 1.0.0 LANGUAGES CXX)

# Set CMake policy for Boost finding (if supported)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Logging configuration options
option(ENABLE_LOGGING "Enable logging system" ON)

# Handle LOG_LEVEL with proper defaults based on build type
if(NOT DEFINED LOG_LEVEL)
    # Set default log levels based on build type when not explicitly set
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(LOG_LEVEL "4" CACHE STRING "Default logging level for Debug builds")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(LOG_LEVEL "2" CACHE STRING "Default logging level for Release builds")
    else()
        set(LOG_LEVEL "3" CACHE STRING "Default logging level: 0=NONE, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG, 5=TRACE")
    endif()
else()
    # User explicitly set LOG_LEVEL, keep their value
    set(LOG_LEVEL "${LOG_LEVEL}" CACHE STRING "User-specified logging level" FORCE)
endif()

# Find required packages
find_package(Boost REQUIRED COMPONENTS graph program_options CONFIG)

# Set up directories
# Set up directories
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Header files
file(GLOB_RECURSE HEADERS 
    "${INCLUDE_DIR}/*.hpp"
    "${INCLUDE_DIR}/*.h"
)

# Create the library
add_library(ggg INTERFACE)

# Set include directories
target_include_directories(ggg 
    INTERFACE
        $<BUILD_INTERFACE:${INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Link Boost libraries (header-only, so INTERFACE)
target_link_libraries(ggg INTERFACE Boost::graph Boost::program_options)

# Configure logging preprocessor definitions
if(ENABLE_LOGGING)
    target_compile_definitions(ggg INTERFACE ENABLE_LOGGING)
    target_compile_definitions(ggg INTERFACE LOG_LEVEL=${LOG_LEVEL})
    message(STATUS "Logging enabled with level ${LOG_LEVEL}")
else()
    message(STATUS "Logging disabled")
endif()

# Compiler-specific options
target_compile_features(ggg INTERFACE cxx_std_20)

# Optional: Build all solvers
# Build options
option(BUILD_ALL_SOLVERS "Build all solver binaries" OFF)
option(BUILD_TESTING "Build unit tests" OFF)
option(BUILD_TOOLS "Build command-line tools" OFF)

# Enable testing early if requested
if(BUILD_TESTING)
    enable_testing()
endif()

if(BUILD_ALL_SOLVERS)
    message(STATUS "Building all solver binaries")
    
    # Create aggregated solver library
    add_library(solvers INTERFACE)
    
    add_subdirectory(solvers/parity/recursive)
    add_subdirectory(solvers/parity/priority_promotion)
    add_subdirectory(solvers/meanpayoff/mse)
    add_subdirectory(solvers/meanpayoff/msca)
    add_subdirectory(solvers/parity/buchi)
    add_subdirectory(solvers/parity/reachability)
    add_subdirectory(solvers/discounted/objective)
    add_subdirectory(solvers/discounted/strategy)
    add_subdirectory(solvers/discounted/value)
    add_subdirectory(solvers/stochastic_discounted/objective)
    add_subdirectory(solvers/stochastic_discounted/strategy)
    add_subdirectory(solvers/stochastic_discounted/value)
    add_subdirectory(solvers/parity/progressive_small_progress_measures)
endif()

# Add main tests (after solvers so targets exist)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Optional: Build tools
option(BUILD_TOOLS "Build command-line tools" OFF)
if(BUILD_TOOLS)
    message(STATUS "Building command-line tools")
    add_subdirectory(tools)
endif()

# Installation
install(TARGETS ggg
    EXPORT GameGraphGymTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY ${INCLUDE_DIR}/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

install(EXPORT GameGraphGymTargets
    FILE GameGraphGymTargets.cmake
    NAMESPACE GameGraphGym::
    DESTINATION lib/cmake/GameGraphGym
)

# Create config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    GameGraphGymConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GameGraphGymConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/GameGraphGymConfig.cmake
    INSTALL_DESTINATION lib/cmake/GameGraphGym
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/GameGraphGymConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/GameGraphGymConfigVersion.cmake
    DESTINATION lib/cmake/GameGraphGym
)
