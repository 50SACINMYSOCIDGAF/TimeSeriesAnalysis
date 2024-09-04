cmake_minimum_required(VERSION 3.14)
project(TimeSeriesAnalysisTool VERSION 1.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ -static -lpthread")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static -lpthread")
endif()

# Find required packages
find_package(CURL CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Print CURL information for debugging
if(CURL_FOUND)
    message(STATUS "CURL found:")
    message(STATUS "  Version: ${CURL_VERSION}")
    message(STATUS "  Libraries: ${CURL_LIBRARIES}")
    message(STATUS "  Include dirs: ${CURL_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "CURL not found. Please make sure it's installed via vcpkg in CLion.")
endif()

# Add the executable
add_executable(time_series_analyzer main.cpp)

# Link libraries
target_link_libraries(time_series_analyzer
        PRIVATE
        CURL::libcurl
        nlohmann_json::nlohmann_json
)

# Set compile options
if(MSVC)
    target_compile_options(time_series_analyzer PRIVATE /W4)
else()
    target_compile_options(time_series_analyzer PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install
install(TARGETS time_series_analyzer DESTINATION bin)

# Testing (optional)
enable_testing()
add_test(NAME TimeSeriesAnalyzerTest COMMAND time_series_analyzer)