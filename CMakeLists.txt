cmake_minimum_required(VERSION 3.17)
project(Kvalog CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find spdlog
find_package(spdlog REQUIRED)

# Find nlohmann_json
find_package(nlohmann_json REQUIRED)

# Header-only library interface
add_library(kvalog INTERFACE)
target_include_directories(kvalog INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(kvalog INTERFACE
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)
target_compile_features(kvalog INTERFACE cxx_std_20)

# Example executable
add_executable(kvalog_example examples/kvalog_example.cpp)
target_link_libraries(kvalog_example PRIVATE kvalog)
target_include_directories(kvalog_example PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/kvalog
)

# Optional: Enable additional C++20/23 features
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(kvalog INTERFACE
        -Wall -Wextra -Wpedantic
    )
endif()

# Optional: Install target for use in other projects
install(DIRECTORY include/ DESTINATION include)