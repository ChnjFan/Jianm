cmake_minimum_required(VERSION 3.16)
project(Jianm
    VERSION 0.1.0
    DESCRIPTION "Build your own MQTT broker from scratch"
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Place all executables in <build>/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Default to Release builds if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo"
    )
endif()

# ---------------------------------------------------------------------------
# Compiler warnings
# ---------------------------------------------------------------------------
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# spdlog (header-only, bundled fmt)
# ---------------------------------------------------------------------------
add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/spdlog/fmt/bundled
)
# spdlog automatically defines SPDLOG_HEADER_ONLY when SPDLOG_COMPILED_LIB
# is not set, so we only need to ensure SPDLOG_COMPILED_LIB stays undefined.

# Source modules (library + executable defined in src/CMakeLists.txt)
# ---------------------------------------------------------------------------
add_subdirectory(src)


# ---------------------------------------------------------------------------
# Testing
# ---------------------------------------------------------------------------
option(JIANM_BUILD_TESTS "Build the Jianm test suite" ON)
if(JIANM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# ---------------------------------------------------------------------------
# Install rules
# ---------------------------------------------------------------------------
install(TARGETS jianm RUNTIME DESTINATION bin)
