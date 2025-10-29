cmake_minimum_required(VERSION 3.20)
project(Kvalog VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find spdlog
find_package(spdlog CONFIG REQUIRED)

# Find nlohmann_json
find_package(nlohmann_json CONFIG REQUIRED)

# Header-only library interface
add_library(kvalog INTERFACE)
target_include_directories(kvalog INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(kvalog INTERFACE
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)
target_compile_features(kvalog INTERFACE cxx_std_23)

# Example executable
option(BUILD_SAMPLES "Build samples" OFF)
if(BUILD_SAMPLES)
    add_executable(kvalog_samples samples/kvalog_samples.cpp)
    target_link_libraries(kvalog_samples PRIVATE kvalog)
    target_include_directories(kvalog_samples PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/kvalog
    )
endif()

# Installation rules
include(GNUInstallDirs)
install(TARGETS kvalog
    EXPORT kvalogTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install the header file
install(FILES kvalog/kvalog.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/kvalog
)

# Install CMake config files
install(EXPORT kvalogTargets
    FILE kvalogTargets.cmake
    NAMESPACE kvalog::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kvalog
)

# Create and install Config files
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/kvalogConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/kvalogConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/kvalogConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kvalog
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/kvalogConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/kvalogConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kvalog
)