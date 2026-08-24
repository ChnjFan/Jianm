# ---------------------------------------------------------------------------
# Asio (standalone, header-only)
# ---------------------------------------------------------------------------
find_path(ASIO_INCLUDE_DIR asio.hpp)
if(ASIO_INCLUDE_DIR)
    message(STATUS "Found Asio: ${ASIO_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "Asio not found. Install it with: brew install asio")
endif()

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${ASIO_INCLUDE_DIR})
target_compile_definitions(asio INTERFACE ASIO_STANDALONE)
target_compile_features(asio INTERFACE cxx_std_17)

# ---------------------------------------------------------------------------
# Jianm source modules
# ---------------------------------------------------------------------------

# Collect source files from the library modules (everything under src/
# except main.cpp, which is the executable's entry point).
# As net/ and protocol/ grow, their files are picked up automatically.
file(GLOB_RECURSE JIANM_LIB_SOURCES
    CONFIGURE_DEPENDS
    common/*.cpp
    common/*.h
    common/*.hpp
    net/*.cpp
    net/*.h
    net/*.hpp
    protocol/*.cpp
    protocol/*.h
    protocol/*.hpp
    broker/*.cpp
    broker/*.h
    broker/*.hpp
)

if(JIANM_LIB_SOURCES)
    # A static library that every module contributes to and that the
    # executable and the test suite both link against.
    add_library(jianm_lib STATIC ${JIANM_LIB_SOURCES})
    target_include_directories(jianm_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(jianm_lib PUBLIC asio spdlog)
    set(JIANM_LINK_LIBS jianm_lib)
else()
    # No module sources yet — provide an INTERFACE target so the executable
    # and tests can still link against the include directory uniformly.
    add_library(jianm_lib INTERFACE)
    target_include_directories(jianm_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(jianm_lib INTERFACE asio spdlog)
    set(JIANM_LINK_LIBS)
endif()

# ---------------------------------------------------------------------------
# Main executable
# ---------------------------------------------------------------------------
add_executable(jianm main.cpp)
target_link_libraries(jianm PRIVATE ${JIANM_LINK_LIBS})

# Copy config file to build output directory so the binary can find it.
# configure_file creates a dependency on the source file: when jianm.conf
# is modified, CMake re-runs automatically and re-copies it.
configure_file(${CMAKE_SOURCE_DIR}/conf/jianm.conf
               ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/jianm.conf COPYONLY)
